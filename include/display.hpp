#pragma once
#include <LovyanGFX.hpp>
#include <Wire.h>

#ifndef RALLY_TFT_INVERT
#define RALLY_TFT_INVERT 0
#endif
#ifndef RALLY_TFT_BGR
#define RALLY_TFT_BGR 0
#endif
#ifndef RALLY_TOUCH_FLIP_X
#define RALLY_TOUCH_FLIP_X 0
#endif
#ifndef RALLY_TOUCH_FLIP_Y
#define RALLY_TOUCH_FLIP_Y 0
#endif

class RallyDisplay : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789 panel;
  lgfx::Bus_SPI bus;
 public:
  RallyDisplay(){
    auto b=bus.config();
    b.spi_host=VSPI_HOST;b.spi_mode=0;b.freq_write=40000000;b.freq_read=16000000;
    b.spi_3wire=false;b.use_lock=true;b.dma_channel=1;
    b.pin_sclk=14;b.pin_mosi=13;b.pin_miso=-1;b.pin_dc=2;
    bus.config(b);panel.setBus(&bus);
    auto p=panel.config();p.pin_cs=15;p.pin_rst=-1;p.pin_busy=-1;
    p.panel_width=240;p.panel_height=320;p.offset_rotation=0;
    p.readable=false;p.invert=RALLY_TFT_INVERT;p.rgb_order=RALLY_TFT_BGR;p.bus_shared=false;
    panel.config(p);setPanel(&panel);
  }
  void begin(){init();setRotation(0);setSwapBytes(true);pinMode(27,OUTPUT);digitalWrite(27,HIGH);}
};

class RallyTouch {
 public:
  bool begin(){
    pinMode(25,OUTPUT);digitalWrite(25,LOW);delay(10);digitalWrite(25,HIGH);delay(100);
    Wire.begin(33,32,400000);Wire.setTimeOut(8);
    Wire.beginTransmission(0x15);return Wire.endTransmission()==0;
  }
  bool read(int &x,int &y){
    Wire.beginTransmission(0x15);Wire.write(0x02);
    if(Wire.endTransmission(false)!=0||Wire.requestFrom(uint8_t(0x15),uint8_t(5))!=5)return false;
    uint8_t n=Wire.read(),xh=Wire.read(),xl=Wire.read(),yh=Wire.read(),yl=Wire.read();
    if((n&15)==0||(xh>>6)==1)return false; // Explicit lift event must not leave steering held.
    x=((xh&15)<<8)|xl;y=((yh&15)<<8)|yl;
    if(x>=240||y>=320)return false;
    if(RALLY_TOUCH_FLIP_X)x=239-x;if(RALLY_TOUCH_FLIP_Y)y=319-y;
    return true;
  }
};
