#include "render.hpp"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <utility>
int main(int argc,char **argv){
  if(argc!=2)return 1;
  std::ofstream out(argv[1]);rally::Game game;rally::Renderer r;r.init(game.track);
  out<<"{\"coverageSections\":"<<rally::COVERAGE_SECTIONS;
  // Shared constants, so the browser port never keeps its own copies.
  out<<",\"rules\":{\"roadHalf\":"<<rally::ROAD_HALF<<",\"pickups\":"<<rally::PICKUPS<<",\"maxUpgrades\":"<<rally::MAX_UPGRADES<<",\"upgradeGain\":"<<rally::UPGRADE_GAIN<<",\"pickupRadius\":"<<rally::PICKUP_RADIUS<<",\"pickupSpread\":"<<rally::PICKUP_SPREAD<<",\"pickupRespawn\":"<<rally::PICKUP_RESPAWN<<",\"backtrackSections\":"<<rally::BACKTRACK_SECTIONS<<",\"maxSpeed\":"<<rally::MAX_SPEED<<"}";
  out<<",\"layout\":{\"turboZone\":"<<rally::TURBO_ZONE<<",\"viewTop\":"<<rally::VIEW_TOP<<",\"carY\":"<<rally::CAR_Y<<",\"depthScale\":"<<rally::DEPTH_SCALE<<"}";
  const std::pair<const char*,uint16_t> palette[]={{"navy",rally::NAVY},{"cream",rally::CREAM},{"cyan",rally::CYAN},{"coral",rally::CORAL},{"green",rally::GREEN},{"darkGreen",rally::DARKGREEN},{"road",rally::ROAD},{"litBlue",rally::LIT_BLUE},{"litCoral",rally::LIT_CORAL},{"halo",rally::HALO},{"dust",rally::DUST},{"mapDim",rally::MAP_DIM},{"boost",rally::BOOST},{"slate",rally::SLATE}};
  out<<",\"palette\":{";for(size_t i=0;i<sizeof(palette)/sizeof(*palette);i++)out<<(i?",":"")<<'"'<<palette[i].first<<"\":"<<palette[i].second;out<<'}';
  out<<",\"track\":[";
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
