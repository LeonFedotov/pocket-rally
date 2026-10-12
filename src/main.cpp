#include <Arduino.h>
#include "display.hpp"
#include "game.hpp"
#include "render.hpp"
#include "link.hpp"

RallyDisplay display;
RallyTouch touch;
rally::Game game;
rally::Renderer renderer;
rally::Link rallyLink;
rally::View sharedView;
portMUX_TYPE viewMux=portMUX_INITIALIZER_UNLOCKED;
bool solo=false,radioOk=false,touchOk=false;
int8_t steering=0;
uint8_t screen=0;

void drawTask(void *){
  // Small scanline buffer expands the 120x160 artwork exactly 2x, without PSRAM.
  uint16_t rows[240*2];
  TickType_t wake=xTaskGetTickCount();
  for(;;){
    rally::View view;
    portENTER_CRITICAL(&viewMux);view=sharedView;portEXIT_CRITICAL(&viewMux);
    renderer.draw(view);
    display.startWrite();
    for(int y=0;y<160;y++){
      for(int x=0;x<120;x++){auto c=renderer.pixels[y*120+x];rows[x*2]=rows[x*2+1]=rows[240+x*2]=rows[241+x*2]=c;}
      display.pushImage(0,y*2,240,2,rows);
      if(y%20==0)taskYIELD();
    }
    display.endWrite();
    vTaskDelay(1); // Always let the idle task run, even if a frame takes over 33 ms.
    vTaskDelayUntil(&wake,pdMS_TO_TICKS(33));
  }
}

void publish(){
  rally::View view;view.race=game.state;view.screen=screen;view.local=rallyLink.role==rally::Link::GUEST?1:0;
  view.ready=rallyLink.readyMask();view.linked=rallyLink.fresh();view.solo=solo;view.steer=steering;
  view.paused=!solo&&screen==4&&(!rallyLink.fresh()||rallyLink.hostPaused);view.error=!touchOk?1:2;
  portENTER_CRITICAL(&viewMux);sharedView=view;portEXIT_CRITICAL(&viewMux);
}

void menu(){solo=false;screen=0;steering=0;rallyLink.choose(rally::Link::NONE);game=rally::Game();}

void turboTap(){int local=rallyLink.role==rally::Link::GUEST?1:0;if(screen==4&&game.state.phase==rally::RACING&&game.state.cars[local].cooldown==0)rallyLink.localTurbo++;}

void onTap(int x,int y){
  if(screen==0){
    if(y>=79&&y<95){rallyLink.choose(rally::Link::HOST);screen=1;}
    else if(y>=97&&y<113){rallyLink.choose(rally::Link::GUEST);screen=2;}
    else if(y>=114&&y<137){solo=true;game.reset(esp_random());screen=4;}
  }else if(screen==1||screen==2)menu();
  else if(screen==3)rallyLink.localReady=true;
  else if(screen==4&&game.state.phase==rally::FINISHED){if(solo)game.reset(esp_random());else rallyLink.localReady=true;}
  else if(screen==4&&x>=40&&x<80&&y>=138)turboTap();
}

void setup(){
  Serial.begin(115200);pinMode(0,INPUT_PULLUP);
  display.begin();touchOk=touch.begin();
#ifdef RALLY_DIAGNOSTIC
  display.fillScreen(TFT_BLACK);display.setTextColor(TFT_WHITE);display.setTextSize(2);
  display.setCursor(8,8);display.print("POCKET RALLY\nTouch test");
  display.fillRect(10,70,65,40,TFT_RED);display.fillRect(85,70,65,40,TFT_GREEN);display.fillRect(160,70,65,40,TFT_BLUE);
  Serial.printf("Touch at 0x15: %s\n",touchOk?"found":"not found");
#else
  radioOk=rallyLink.begin();renderer.init(game.track);
  if(!touchOk||!radioOk)screen=5;
  publish();
  if(xTaskCreatePinnedToCore(drawTask,"rally-render",6144,nullptr,1,nullptr,0)!=pdPASS){Serial.println("Renderer task allocation failed");display.fillScreen(TFT_RED);return;}
  Serial.printf("POCKET RALLY 0.5 | %s | touch=%d radio=%d | free heap=%u\n",WiFi.macAddress().c_str(),touchOk,radioOk,ESP.getFreeHeap());
  Serial.println("Hold BOOT for 1 second to return to menu. Serial h=host j=join p=practice r=ready m=menu.");
#endif
}

void loop(){
  static uint32_t inputAt=0,simulationAt=micros(),bootAt=0;static bool wasTouched=false,bootHandled=false;
  uint32_t now=millis();int x=0,y=0;
#ifdef RALLY_DIAGNOSTIC
  if(touch.read(x,y)){display.fillCircle(x,y,3,TFT_YELLOW);Serial.printf("touch %d,%d\n",x,y);}
  delay(15);return;
#else
  if(!radioOk||!touchOk){delay(20);return;}
  if(now-inputAt>=8){
    inputAt=now;bool pressed=touch.read(x,y);
    if(pressed&&!wasTouched)onTap(x/2,y/2);
    // One finger: tap turbo, then slide to either steering zone while it runs.
    bool turboZone=x>=80&&x<160&&y>=276;
    steering=pressed&&screen==4&&game.state.phase!=rally::FINISHED&&!turboZone?(x<120?-1:1):0;
    wasTouched=pressed;
  }
  if(digitalRead(0)==LOW){if(!bootAt)bootAt=now;if(!bootHandled&&now-bootAt>1000){menu();bootHandled=true;}}
  else {bootAt=0;bootHandled=false;}
  while(Serial.available()){
    char c=Serial.read();if(c=='h'){rallyLink.choose(rally::Link::HOST);screen=1;}if(c=='j'){rallyLink.choose(rally::Link::GUEST);screen=2;}
    if(c=='p'){solo=true;rallyLink.choose(rally::Link::NONE);game.reset(esp_random());screen=4;}
    if(c=='r')rallyLink.localReady=true;if(c=='m')menu();if(c=='t')turboTap();
  }
  rallyLink.pump(game,steering);
  if(!solo&&rallyLink.paired){screen=game.state.phase==rally::LOBBY?3:4;}
  if(rallyLink.role==rally::Link::HOST&&rallyLink.fresh()&&(game.state.phase==rally::LOBBY||game.state.phase==rally::FINISHED)&&rallyLink.readyMask()==3){game.reset(esp_random());rallyLink.clearReady();screen=4;}
  // Fixed physics tick. Avoid a long catch-up burst after USB/logging stalls.
  uint32_t us=micros();if(uint32_t(us-simulationAt)>100000)simulationAt=us-16667;
  while(uint32_t(us-simulationAt)>=16667){
    simulationAt+=16667;
    if(screen==4&&(solo||(rallyLink.fresh()&&!rallyLink.hostPaused))){
      if(solo){if(game.botTurbo(1))rallyLink.remoteTurbo++;game.step(1.f/60,steering,game.bot(1),rallyLink.localTurbo,rallyLink.remoteTurbo);}
      else if(rallyLink.role==rally::Link::HOST)game.step(1.f/60,steering,uint32_t(now-rallyLink.lastRx)<150?rallyLink.remoteSteer:0,rallyLink.localTurbo,rallyLink.remoteTurbo);
      else {
        // Predict between authoritative snapshots for immediate steering feedback.
        auto phase=game.state.phase;game.step(1.f/60,rallyLink.remoteSteer,steering,rallyLink.remoteTurbo,rallyLink.localTurbo);
        if(phase!=rally::FINISHED&&game.state.phase==rally::FINISHED){game.state.phase=rally::RACING;game.state.winner=255;}
      }
    }
  }
  publish();delay(1);
#endif
}
