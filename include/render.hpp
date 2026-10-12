#pragma once
#include "game.hpp"
#include <cstdio>

namespace rally {
constexpr uint16_t rgb(unsigned r,unsigned g,unsigned b){return uint16_t(((r&248)<<8)|((g&252)<<3)|(b>>3));}
constexpr uint16_t NAVY=rgb(17,16,27),CREAM=rgb(246,240,229),CYAN=rgb(107,231,218),CORAL=rgb(255,146,158),GREEN=rgb(23,21,36),DARKGREEN=rgb(59,55,78),ROAD=rgb(67,60,89);
constexpr uint16_t LIT_BLUE=rgb(56,132,137),LIT_CORAL=rgb(154,82,113),HALO=rgb(35,30,50),DUST=rgb(66,55,83),MAP_DIM=rgb(158,143,177);
constexpr int MAP_X=82,MAP_Y=19,MAP_W=35,MAP_H=44;
struct View {
  State race; uint8_t screen=0,local=0,ready=0; bool linked=false,paused=false,solo=false;
  int8_t steer=0; uint8_t error=0;
};
struct Glyph {char ch;const char *bits;};
const Glyph glyphs[]={
 {'A',"010101111101101"},{'B',"110101110101110"},{'C',"011100100100011"},{'D',"110101101101110"},{'E',"111100110100111"},{'F',"111100110100100"},{'G',"011100101101011"},{'H',"101101111101101"},{'I',"111010010010111"},{'J',"001001001101010"},{'K',"101101110101101"},{'L',"100100100100111"},{'M',"101111111101101"},{'N',"101111111111101"},{'O',"010101101101010"},{'P',"110101110100100"},{'Q',"010101101011001"},{'R',"110101110101101"},{'S',"011100010001110"},{'T',"111010010010010"},{'U',"101101101101111"},{'V',"101101101101010"},{'W',"101101111111101"},{'X',"101101010101101"},{'Y',"101101010010010"},{'Z',"111001010100111"},
 {'0',"111101101101111"},{'1',"010110010010111"},{'2',"110001010100111"},{'3',"110001010001110"},{'4',"101101111001001"},{'5',"111100110001110"},{'6',"011100111101111"},{'7',"111001010010010"},{'8',"111101111101111"},{'9',"111101111001110"},{'/',"001001010100100"},{':',"000010000010000"},{'.',"000000000000010"},{'-',"000000111000000"},{'!',"010010010000010"},{'%',"101001010100101"}
};

class Renderer {
 public:
  uint16_t pixels[W*H]{};
  uint8_t roadDepth[W*123]{};
  Track track;
  Vec mapCenter{};float mapScale=1;
  Vec coveragePoints[COVERAGE_SECTIONS+1]{};
  void pixel(int x,int y,uint16_t color){if(x>=0&&x<W&&y>=0&&y<H)pixels[y*W+x]=color;}
  void rect(int x,int y,int w,int h,uint16_t c){for(int yy=std::max(0,y);yy<std::min(H,y+h);yy++)for(int xx=std::max(0,x);xx<std::min(W,x+w);xx++)pixels[yy*W+xx]=c;}
  void label(const char *txt,int x,int y,uint16_t c,int size=1){
    for(;*txt;txt++,x+=4*size)for(const auto &g:glyphs)if(g.ch==*txt){for(int k=0;k<15;k++)if(g.bits[k]=='1')rect(x+(k%3)*size,y+(k/3)*size,size,size,c);break;}
  }
  void center(const char *txt,int y,uint16_t c,int size=1){label(txt,(W-(int(std::strlen(txt))*4-1)*size)/2,y,c,size);}
  void outlinedLabel(const char *txt,int x,int y,uint16_t c){label(txt,x-1,y,NAVY);label(txt,x+1,y,NAVY);label(txt,x,y-1,NAVY);label(txt,x,y+1,NAVY);label(txt,x,y,c);}
  // Small geometric ghosts, also used for the off-screen peer cue.
  void ghost(int px,int py,int player){
    for(int y=-3;y<=3;y++)for(int x=-3;x<=3;x++){
      const int d=std::abs(x)+std::abs(y);
      if(d<=3)pixel(px+x,py+y,d==3?NAVY:d==2?(player?CORAL:CYAN):d==0?CREAM:player?NAVY:CYAN);
    }
  }
  void init(const Track &source){
    track=source;Vec lo=track.points[0],hi=lo;
    for(int i=1;i<track.count;i++){const auto p=track.points[i];lo.x=std::min(lo.x,p.x);lo.y=std::min(lo.y,p.y);hi.x=std::max(hi.x,p.x);hi.y=std::max(hi.y,p.y);}
    mapCenter={(lo.x+hi.x)*.5f,(lo.y+hi.y)*.5f};
    mapScale=std::min((MAP_W-10)/std::max(1.f,hi.x-lo.x),(MAP_H-15)/std::max(1.f,hi.y-lo.y));
    for(int i=0;i<=COVERAGE_SECTIONS;i++)coveragePoints[i]=track.at(track.length*i/COVERAGE_SECTIONS).point;
  }
  Vec mapPoint(Vec p) const {return {MAP_X+MAP_W*.5f+(p.x-mapCenter.x)*mapScale,MAP_Y+4+(MAP_H-15)*.5f+(p.y-mapCenter.y)*mapScale};}
  void mapLine(Vec a,Vec b,uint16_t color){
    int x=std::lround(a.x),y=std::lround(a.y),tx=std::lround(b.x),ty=std::lround(b.y);
    int dx=std::abs(tx-x),dy=-std::abs(ty-y),sx=x<tx?1:-1,sy=y<ty?1:-1,error=dx+dy;
    for(;;){pixel(x,y,color);if(x==tx&&y==ty)break;int e=2*error;if(e>=dy){error+=dy;x+=sx;}if(e<=dx){error+=dx;y+=sy;}}
  }
  void miniMap(const State &s,int local){
    // Only the ribbon and glyphs are painted; no background panel.
    for(int i=0;i<track.count-1;i++)segment(mapPoint(track.points[i]),mapPoint(track.points[i+1]),1.5f,NAVY);
    for(int i=0;i<COVERAGE_SECTIONS;i++)mapLine(mapPoint(coveragePoints[i]),mapPoint(coveragePoints[i+1]),coveredSection(s.cars[local],i)?(local?CORAL:CYAN):MAP_DIM);
    // Draw unvisited centres last so even a short missed section remains visible.
    for(int i=0;i<COVERAGE_SECTIONS;i++)if(!coveredSection(s.cars[local],i)){
      auto p=mapPoint({(coveragePoints[i].x+coveragePoints[i+1].x)*.5f,(coveragePoints[i].y+coveragePoints[i+1].y)*.5f});pixel(std::lround(p.x),std::lround(p.y),MAP_DIM);
    }
    Vec actual[2],marker[2];
    for(int i=0;i<2;i++){
      actual[i]=mapPoint({s.cars[i].x,s.cars[i].y});
      actual[i].x=clamp(actual[i].x,MAP_X+4,MAP_X+MAP_W-5);actual[i].y=clamp(actual[i].y,MAP_Y+4,MAP_Y+MAP_H-13);marker[i]=actual[i];
    }
    // When icons would overlap, separate them with leaders to their real positions.
    if(std::fabs(marker[0].x-marker[1].x)<6&&std::fabs(marker[0].y-marker[1].y)<6){
      float middle=clamp((marker[0].x+marker[1].x)*.5f,MAP_X+7,MAP_X+MAP_W-8);
      marker[0].x=middle-3;marker[1].x=middle+3;
      for(int i=0;i<2;i++)mapLine(actual[i],marker[i],i?CORAL:CYAN);
    }
    for(int i=0;i<2;i++)ghost(std::lround(marker[i].x),std::lround(marker[i].y),i);
    outlinedLabel("P1",MAP_X+5,MAP_Y+MAP_H-7,CYAN);outlinedLabel("P2",MAP_X+22,MAP_Y+MAP_H-7,CORAL);
    rect(MAP_X+(local?21:4),MAP_Y+MAP_H-2,9,3,NAVY);
    rect(MAP_X+(local?22:5),MAP_Y+MAP_H-1,7,1,CREAM);
  }
  // Rasterize only the visible capsules; no full-world framebuffer or PSRAM.
  void segment(Vec a,Vec b,float radius,uint16_t color,bool nearest=false){
    int x0=std::max(0,int(std::floor(std::min(a.x,b.x)-radius))),x1=std::min(W-1,int(std::ceil(std::max(a.x,b.x)+radius)));
    int y0=std::max(15,int(std::floor(std::min(a.y,b.y)-radius))),y1=std::min(137,int(std::ceil(std::max(a.y,b.y)+radius)));
    if(x0>x1||y0>y1)return;
    float dx=b.x-a.x,dy=b.y-a.y,l2=dx*dx+dy*dy;
    for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++){
      float t=l2>0?((x-a.x)*dx+(y-a.y)*dy)/l2:0;
      t=clamp(t,0,1);
      float xx=x-a.x-dx*t,yy=y-a.y-dy*t,d2=xx*xx+yy*yy;
      if(d2<=radius*radius){
        if(nearest){auto &depth=roadDepth[(y-15)*W+x];uint8_t score=uint8_t(d2+.5f);if(score>depth)continue;depth=score;}
        pixel(x,y,color);
      }
    }
  }
  static Vec camera(Vec p,const Car &focus){
    float dx=p.x-focus.x,dy=p.y-focus.y,cs=std::cos(focus.angle),sn=std::sin(focus.angle);
    return {60-dx*sn+dy*cs,103-dx*cs-dy*sn};
  }
  void car(const Car &car,int player){
    static const char *sprites[2][11]={
      {"000030000","000232000","000222000","002222200","022232220","222333222","022212220","002111200","000202000","000202000","000000000"},
      {"000030000","000232000","002212200","022111220","221131122","211333112","022111220","002212200","000202000","000202000","000000000"}
    };
    const auto *sprite=sprites[player];
    const uint16_t palette[]={0,NAVY,player?CORAL:CYAN,CREAM};
    float angle=std::round(car.angle/(PI_F/8))*(PI_F/8)+PI_F/2,cs=std::cos(angle),sn=std::sin(angle);
    int cx=std::lround(car.x),cy=std::lround(car.y);
    for(int y=-7;y<=7;y++)for(int x=-7;x<=7;x++){
      int sx=std::lround(cs*x+sn*y)+4,sy=std::lround(-sn*x+cs*y)+5;
      if(sx>=0&&sx<9&&sy>=0&&sy<11&&sprite[sy][sx]!='0')pixel(cx+x,cy+y,palette[sprite[sy][sx]-'0']);
    }
  }
  void world(const State &s,int local){
    std::fill(pixels,pixels+W*H,GREEN);const auto &focus=s.cars[local];Vec projected[97];
    std::memset(roadDepth,255,sizeof(roadDepth));
    // World-anchored specks and open diamonds give motion a quiet visual reference.
    for(int row=0;row<8;row++)for(int col=0;col<8;col++){
      int seed=col*37+row*53;Vec p=camera({float(col*72+seed%29),float(row*72+(seed*3)%31)},focus);
      int x=std::lround(p.x),y=std::lround(p.y);
      if(seed%3==0){pixel(x-2,y,DUST);pixel(x+2,y,DUST);pixel(x,y-2,DUST);pixel(x,y+2,DUST);}else pixel(x,y,DUST);
    }
    for(int i=0;i<track.count;i++)projected[i]=camera(track.points[i],focus);
    for(int i=0;i<track.count-1;i++)segment(projected[i],projected[i+1],ROAD_HALF+3,HALO);
    // Closest-section colouring joins bends without seams or lighting across gaps.
    for(int i=0;i<COVERAGE_SECTIONS;i++)segment(camera(coveragePoints[i],focus),camera(coveragePoints[i+1],focus),ROAD_HALF,coveredSection(focus,i)?(local?LIT_CORAL:LIT_BLUE):ROAD,true);
    // Pixel-dissolved edges make a soft colour ribbon without curbs or lane marks.
    for(int y=15;y<138;y++)for(int x=0;x<W;x++){
      auto depth=roadDepth[(y-15)*W+x];
      if(depth>=110&&depth!=255&&((x+y)&1))pixel(x,y,HALO);
    }
    for(int i=0;i<2;i++){
      auto c=s.cars[i];Vec p=camera({c.x,c.y},focus);c.x=p.x;c.y=p.y;c.angle=wrap(c.angle-focus.angle-PI_F/2);
      float dx=std::cos(c.angle),dy=std::sin(c.angle);
      for(int k=0;k<(c.boost>0?4:2);k++){
        float d=9+k*4;int x=std::lround(c.x-dx*d),y=std::lround(c.y-dy*d);
        rect(x,y,k==0?2:1,k==0?2:1,c.boost>0?(i?CORAL:CYAN):(i?LIT_CORAL:LIT_BLUE));
      }
      car(c,i);
    }
    // A geometric ghost keeps the opponent visible outside the camera.
    Vec other=camera({s.cars[1-local].x,s.cars[1-local].y},focus);
    if(other.x<5||other.x>114||other.y<21||other.y>128){
      int x=int(clamp(other.x,6,108)),y=int(clamp(other.y,23,126));
      ghost(x,y,1-local);
    }
  }
  void button(int y,const char *text,uint16_t color){rect(17,y,86,16,color);rect(18,y+1,84,14,NAVY);center(text,y+5,CREAM);}
  void panel(int y,int height){rect(10,y,100,height,NAVY);rect(11,y+1,98,1,DUST);}
  void draw(const View &view){
    const auto &s=view.race;world(s,view.local);
    char txt[32];uint16_t color=view.local?CORAL:CYAN;const auto &c=s.cars[view.local];
    rect(0,0,W,15,NAVY);rect(0,138,W,22,NAVY);
    std::snprintf(txt,sizeof(txt),"P%u",view.local+1);label(txt,4,5,color);
    std::snprintf(txt,sizeof(txt),"LAP %u/3",std::min(3,int(c.lap)+1));label(txt,24,5,CREAM);
    std::snprintf(txt,sizeof(txt),"%d%%",coveragePercent(c));label(txt,64,5,color);
    label(view.solo?"SOLO":"LINK",94,5,view.linked||view.solo?CREAM:rgb(124,134,131));
    for(int i=0;i<2;i++){
      int x=i?82:2;rect(x,140,36,18,view.steer==(i?1:-1)?color:ROAD);
      int cx=x+18;for(int j=0;j<4;j++)rect(cx+(i?j:-j)-1,145+j-1,3,8-2*j,CREAM);
    }
    rect(41,140,38,18,c.cooldown==0||c.boost>0?color:ROAD);
    center(c.boost>0?"TURBO!":c.cooldown>0?"CHARGE":"TURBO",144,c.cooldown==0||c.boost>0?NAVY:CREAM);
    float fill=c.boost>0?c.boost/TURBO_TIME:c.cooldown>0?1-c.cooldown/TURBO_RECOVERY:1;
    rect(44,153,32,2,NAVY);rect(44,153,int(32*clamp(fill,0,1)),2,CREAM);
    if(view.screen==4&&s.phase==RACING&&!view.paused)miniMap(s,view.local);
    if(view.screen==0){
      panel(43,86);center("POCKET",49,CREAM,2);center("RALLY",62,CORAL,2);
      button(79,"HOST RACE",CYAN);button(97,"JOIN RACE",CORAL);
      center("PRACTICE",118,CREAM);
    }else if(view.screen==1||view.screen==2){
      panel(50,60);center(view.screen==1?"HOSTING RACE":"FINDING HOST",60,CREAM);
      center(view.screen==1?"JOIN ON P2":"KEEP BOARDS NEAR",73,CYAN);
      center("TAP TO CANCEL",95,CREAM);
    }else if(view.screen==3){
      panel(47,69);center("PLAYERS LINKED",56,CYAN);center("3 LAPS",69,CREAM);
      bool ready=view.ready&(1<<view.local);button(82,ready?"WAITING FOR PEER":"TAP WHEN READY",ready?DARKGREEN:CORAL);
      if(ready)center("BOTH MUST READY",105,CREAM);
    }else if(view.screen==5){
      panel(48,65);center("SETUP ERROR",59,CORAL);center(view.error==1?"TOUCH NOT FOUND":"RADIO FAILED",76,CREAM);center("SEE SERIAL LOG",94,CREAM);
    }else if(view.paused){
      panel(57,47);center("LINK PAUSED",67,CORAL);center("RECONNECTING",86,CREAM);
    }else if(s.phase==COUNTDOWN){
      panel(58,41);std::snprintf(txt,sizeof(txt),"%d",std::max(1,int(std::ceil(s.countdown))));center(txt,65,CREAM,4);center("GET READY",88,CYAN);
    }else if(s.phase==FINISHED){
      panel(48,69);center(s.winner==2?"PHOTO FINISH":s.winner==view.local?"YOU WIN!":"P2 WINS!",57,s.winner==view.local?CYAN:CORAL);
      if(s.winner!=view.local&&s.winner<2){rect(13,55,94,11,NAVY);std::snprintf(txt,sizeof(txt),"P%u WINS!",s.winner+1);center(txt,58,CORAL);}
      center("NICE RACING",74,CREAM);button(89,(view.ready&(1<<view.local))?"REMATCH READY":"TAP FOR REMATCH",CYAN);
    }
  }
};
}
