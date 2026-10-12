#include "render.hpp"
#include <fstream>
#include <iostream>
#include <iomanip>
int main(int argc,char **argv){
  if(argc!=2)return 1;
  std::ofstream out(argv[1]);rally::Game game;rally::Renderer r;r.init(game.track);
  out<<"{\"coverageSections\":"<<rally::COVERAGE_SECTIONS<<",\"track\":[";
  for(int i=0;i<game.track.count;i++){if(i)out<<',';out<<'['<<std::fixed<<std::setprecision(5)<<game.track.points[i].x<<','<<game.track.points[i].y<<']';}
  out<<"],\"map\":{\"x\":"<<rally::MAP_X<<",\"y\":"<<rally::MAP_Y<<",\"w\":"<<rally::MAP_W<<",\"h\":"<<rally::MAP_H<<",\"cx\":"<<r.mapCenter.x<<",\"cy\":"<<r.mapCenter.y<<",\"scale\":"<<r.mapScale<<"},\"glyphs\":{";
  bool first=true;for(const auto &g:rally::glyphs){if(!first)out<<',';first=false;out<<'"'<<g.ch<<"\":\""<<g.bits<<'"';}
  out<<"},\"cars\":[";
  for(int p=0;p<2;p++)for(int a=0;a<16;a++){
    if(p||a)out<<',';out<<'[';std::memset(r.pixels,0,sizeof(r.pixels));rally::Car c;c.x=8;c.y=8;c.angle=a*rally::PI_F/8;r.car(c,p);
    for(int y=0;y<18;y++)for(int x=0;x<18;x++){if(x||y)out<<',';out<<r.pixels[y*rally::W+x];}
    out<<']';
  }
  out<<"]}";
}
