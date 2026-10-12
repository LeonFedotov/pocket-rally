#pragma once
#include "Arduino.h"
using esp_now_send_status_t=int;
struct esp_now_peer_info_t {uint8_t peer_addr[6];uint8_t channel;int ifidx;bool encrypt;};
inline void (*fakeSent)(const uint8_t*,esp_now_send_status_t)=nullptr;
inline std::vector<uint8_t> fakeLastPacket;
inline int esp_now_init(){return 0;}
inline void esp_now_register_recv_cb(void (*)(const uint8_t*,const uint8_t*,int)){}
inline void esp_now_register_send_cb(void (*cb)(const uint8_t*,esp_now_send_status_t)){fakeSent=cb;}
inline bool esp_now_is_peer_exist(const uint8_t*){return false;}
inline int esp_now_add_peer(const esp_now_peer_info_t*){return 0;}
inline void esp_now_del_peer(const uint8_t*){}
inline int esp_now_send(const uint8_t *mac,const uint8_t *p,size_t size){fakeLastPacket.assign(p,p+size);if(fakeSent)fakeSent(mac,0);return 0;}
