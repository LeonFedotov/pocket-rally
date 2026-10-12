#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include "game.hpp"

namespace rally {
constexpr uint8_t RADIO_CHANNEL=6,PROTOCOL=4;
constexpr uint32_t MAGIC=0x52414c59;
enum Kind:uint8_t { HELLO=1,JOIN=2,WELCOME=3,CONTROL=4,SNAPSHOT=5 };
struct Packet {
  uint32_t magic=MAGIC,session=0,boot=0,sequence=0;
  uint8_t version=PROTOCOL,kind=0,ready=0; int8_t steer=0; // steer: -100..100
  uint8_t paused=0,reserved=0;uint16_t turboRequest=0;
  State race{};
};
static_assert(sizeof(Packet)<=250,"ESP-NOW v1 limit");
struct Received { uint8_t mac[6]; Packet packet; };
static QueueHandle_t rxQueue=nullptr;
static volatile bool txBusy=false;
static const uint8_t broadcastMac[6]={255,255,255,255,255,255};
// Callback work stays bounded. All validation and game changes happen in loop().
static void receivePacket(const uint8_t *mac,const uint8_t *bytes,int size){
  if(!rxQueue||size!=sizeof(Packet))return;
  Received rx{};std::memcpy(rx.mac,mac,6);std::memcpy(&rx.packet,bytes,sizeof(Packet));
  xQueueSend(rxQueue,&rx,0);
}
static void sentPacket(const uint8_t *,esp_now_send_status_t){txBusy=false;}

class Link {
 public:
  enum Role { NONE,HOST,GUEST } role=NONE;
  bool paired=false,chosen=false,localReady=false,remoteReady=false,hostPaused=false;
  int8_t remoteSteer=0;
  uint16_t localTurbo=0,remoteTurbo=0;
  uint8_t hostReadyMask=0;
  uint32_t session=0,peerBoot=0,boot=0,lastRx=0;
  uint8_t peer[6]{};
  bool begin(){
    rxQueue=xQueueCreate(12,sizeof(Received));if(!rxQueue)return false;
    WiFi.mode(WIFI_STA);WiFi.disconnect();
    esp_wifi_set_ps(WIFI_PS_NONE);
    if(esp_wifi_set_channel(RADIO_CHANNEL,WIFI_SECOND_CHAN_NONE)!=ESP_OK)return false;
    if(esp_now_init()!=ESP_OK)return false;
    esp_now_register_recv_cb(receivePacket);esp_now_register_send_cb(sentPacket);
    boot=esp_random();return add(broadcastMac);
  }
  void choose(Role r){
    if(chosen)esp_now_del_peer(peer);
    role=r;paired=false;chosen=false;localReady=remoteReady=false;hostReadyMask=0;remoteSteer=0;hostPaused=false;
    session=r==HOST?esp_random():0;peerBoot=0;txSeq=rxSeq=0;lastHello=lastTx=lastRx=0;haveRxSeq=false;
    localTurbo=remoteTurbo=0;inputRaceId=0;
    if(rxQueue)xQueueReset(rxQueue);
  }
  bool fresh() const {return paired&&uint32_t(millis()-lastRx)<600;}
  uint8_t readyMask() const {return role==HOST?uint8_t((localReady?1:0)|(remoteReady?2:0)):uint8_t((hostReadyMask&1)|(localReady?2:0));}
  void clearReady(){localReady=remoteReady=false;hostReadyMask=0;}
  void pump(Game &game,int8_t steering){
    Received rx;
    while(xQueueReceive(rxQueue,&rx,0)==pdTRUE)accept(rx,game);
    if(inputRaceId!=game.state.raceId){inputRaceId=game.state.raceId;localTurbo=game.state.cars[role==GUEST?1:0].turboRequest;remoteTurbo=game.state.cars[role==GUEST?0:1].turboRequest;}
    uint32_t now=millis();
    if(role==NONE)return;
    if(role==HOST&&(!paired||!fresh())&&now-lastHello>=300){if(send(HELLO,broadcastMac,game,steering))lastHello=now;}
    if(role==GUEST&&chosen&&!paired&&now-lastTx>=250){if(send(JOIN,peer,game,steering))lastTx=now;}
    if(paired&&now-lastTx>=(role==HOST?33u:16u)){
      if(send(role==HOST?SNAPSHOT:CONTROL,peer,game,steering))lastTx=now;
    }
  }
 private:
  uint32_t txSeq=0,rxSeq=0,lastHello=0,lastTx=0,busySince=0,inputRaceId=0;bool haveRxSeq=false;
  bool add(const uint8_t *mac){
    if(esp_now_is_peer_exist(mac))return true;
    esp_now_peer_info_t info{};std::memcpy(info.peer_addr,mac,6);info.channel=RADIO_CHANNEL;info.ifidx=WIFI_IF_STA;info.encrypt=false;
    return esp_now_add_peer(&info)==ESP_OK;
  }
  bool send(Kind kind,const uint8_t *mac,const Game &game,int8_t steering){
    if(txBusy&&uint32_t(millis()-busySince)<250)return false;
    Packet p{};p.session=session;p.boot=boot;p.sequence=++txSeq;p.kind=kind;
    p.ready=role==HOST?readyMask():(localReady?1:0);p.steer=steering;p.turboRequest=localTurbo;
    p.paused=role==HOST&&!fresh();p.race=game.state;
    txBusy=true;busySince=millis();
    if(esp_now_send(mac,reinterpret_cast<const uint8_t*>(&p),sizeof(p))!=ESP_OK){txBusy=false;return false;}
    return true;
  }
  void accept(const Received &rx,Game &game){
    const Packet &p=rx.packet;
    if(p.magic!=MAGIC||p.version!=PROTOCOL||p.kind<HELLO||p.kind>SNAPSHOT||p.steer<-100||p.steer>100)return;
    if(role==GUEST&&p.kind==HELLO&&(!chosen||std::memcmp(peer,rx.mac,6)==0)){
      if(chosen&&session==p.session&&peerBoot==p.boot)return;
      if(!add(rx.mac))return;
      std::memcpy(peer,rx.mac,6);chosen=true;paired=false;session=p.session;peerBoot=p.boot;
      localReady=false;hostReadyMask=0;haveRxSeq=false;lastTx=0;return;
    }
    if(role==HOST&&p.kind==JOIN&&p.session==session&&(!chosen||std::memcmp(peer,rx.mac,6)==0)){
      if(!add(rx.mac))return;
      if(!chosen||peerBoot!=p.boot){haveRxSeq=false;remoteReady=false;localReady=false;remoteSteer=0;remoteTurbo=game.state.cars[1].turboRequest;}
      std::memcpy(peer,rx.mac,6);peerBoot=p.boot;chosen=paired=true;lastRx=millis();
      send(WELCOME,peer,game,0);return;
    }
    if(!chosen||std::memcmp(peer,rx.mac,6)!=0||p.session!=session||p.boot!=peerBoot)return;
    if(haveRxSeq&&int32_t(p.sequence-rxSeq)<=0)return;
    if(role==HOST&&p.kind==CONTROL&&p.ready<=1){
      haveRxSeq=true;rxSeq=p.sequence;lastRx=millis();remoteSteer=p.steer;
      if(p.race.raceId==game.state.raceId){remoteReady=p.ready!=0;remoteTurbo=p.turboRequest;}
    }else if(role==GUEST&&(p.kind==SNAPSHOT||p.kind==WELCOME)&&p.ready<=3&&p.paused<=1&&validState(p.race)){
      haveRxSeq=true;rxSeq=p.sequence;lastRx=millis();paired=true;remoteSteer=p.steer;
      if(p.race.raceId!=game.state.raceId||p.kind==WELCOME){localReady=false;localTurbo=p.race.cars[1].turboRequest;}
      game.state=p.race;hostReadyMask=p.ready;hostPaused=p.paused!=0;
      remoteTurbo=p.turboRequest;
    }
  }
};
}
