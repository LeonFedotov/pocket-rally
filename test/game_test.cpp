#include "game.hpp"
#include "render.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <limits>

int main(int argc,char **argv){
  rally::Game game;game.reset(42);
  assert(game.track.count==97);assert(game.track.length>1400&&game.track.length<1800);
  std::cout<<"Track length: "<<game.track.length<<" world pixels\n";
  assert(game.track.project(180,70).distance<.001f);
  assert(game.track.project(250,250).distance>15);
  for(int n=0;n<180;n++)game.step(1.f/60,0,0);
  assert(game.state.clock==0);
  bool mistake=false,backtrack=false,offRibbon=false;
  for(int n=0;n<20000&&game.state.phase!=rally::FINISHED;n++){
    game.step(1.f/60,game.bot(0),game.bot(1),game.state.cars[0].turboRequest+game.botTurbo(0),game.state.cars[1].turboRequest+game.botTurbo(1));assert(rally::validState(game.state));
    for(int i=0;i<2;i++){mistake|=game.brains[i].mistake>0;backtrack|=game.brains[i].target>=0;offRibbon|=game.track.project(game.state.cars[i].x,game.state.cars[i].y).distance>rally::ROAD_HALF;}
  }
  assert(game.state.phase==rally::FINISHED);assert(game.state.clock<240);
  assert(mistake&&offRibbon&&backtrack); // Bots err, leave the ribbon and go back for missed sections.
  assert(game.state.cars[game.state.winner==2?0:game.state.winner].lap==3);
  std::cout<<"Bot race: "<<game.state.clock<<"s; winner="<<int(game.state.winner)+1<<"\n";
  auto finished=game.state;game.step(1,1,1);assert(game.state.tick==finished.tick);
  auto bad=finished;bad.cars[0].x=std::numeric_limits<float>::quiet_NaN();assert(!rally::validState(bad));
  game.reset(43);game.state.phase=rally::RACING;
  game.state.cars[0]={180,70,0,20};game.state.cars[1]={180,70,0,20};game.step(1.f/60,0,0);
  assert(std::fabs(game.state.cars[0].y-game.state.cars[1].y)>=6.9f);
  game.reset(44);game.state.phase=rally::RACING;
  // Coverage is personal; repeated visits cannot replace a missing section.
  auto visit=[&](int section){auto p=game.track.at(game.track.length*(section+.5f)/rally::COVERAGE_SECTIONS);auto &c=game.state.cars[0];c.x=p.point.x;c.y=p.point.y;c.angle=std::atan2(p.tangent.y,p.tangent.x);c.speed=0;game.step(1.f/60,0,0);};
  game.state.cars[1].x=250;game.state.cars[1].y=250;
  for(int i=0;i<rally::COVERAGE_SECTIONS;i++)if(i!=47)visit(i);
  assert(game.state.cars[0].lap==0&&game.state.cars[0].covered==rally::COVERAGE_SECTIONS-1);
  assert(game.state.cars[1].covered==0);assert(!rally::coveredSection(game.state.cars[0],47));
  for(int i=0;i<20;i++)visit(0);
  assert(game.state.cars[0].lap==0);
  visit(47);assert(game.state.cars[0].lap==1&&game.state.cars[0].covered==0);
  // Reverse order is allowed; every section still has to be driven.
  for(int i=rally::COVERAGE_SECTIONS-1;i>=0;i--)visit(i);
  assert(game.state.cars[0].lap==2&&game.state.cars[0].covered==0);
  for(int i=0;i<rally::COVERAGE_SECTIONS;i++)visit(i);
  assert(game.state.phase==rally::FINISHED&&game.state.winner==0&&game.state.cars[0].covered==rally::COVERAGE_SECTIONS);
  assert(rally::validState(game.state));
  bad=game.state;bad.cars[0].covered--;assert(!rally::validState(bad));
  // Road and off-road acceleration are identical; only road visits count.
  rally::Game onRoad,offRoad;onRoad.reset(45);offRoad.reset(45);onRoad.state.phase=offRoad.state.phase=rally::RACING;
  onRoad.state.cars[0].x=210;onRoad.state.cars[0].y=70;onRoad.state.cars[0].angle=0;
  offRoad.state.cars[0].x=210;offRoad.state.cars[0].y=215;offRoad.state.cars[0].angle=0;
  for(int n=0;n<40;n++){onRoad.step(1.f/60,0,0);offRoad.step(1.f/60,0,0);assert(std::fabs(onRoad.state.cars[0].speed-offRoad.state.cars[0].speed)<.0001f);}
  assert(offRoad.state.cars[0].covered==0&&onRoad.state.cars[0].covered>0);
  // A turbo tap is consumed once, including duplicate packets and counter wrap.
  game.reset(47);game.state.phase=rally::RACING;
  game.step(1.f/60,0,0,1,0);assert(game.state.cars[0].boost==2);assert(game.state.cars[0].cooldown==7);
  for(int n=0;n<60;n++)game.step(1.f/60,game.bot(0),game.bot(1),1,0);
  assert(game.state.cars[0].boost>.98f&&game.state.cars[0].boost<1.02f);assert(game.state.cars[0].speed>48);
  game.step(1.f/60,0,0,2,0);assert(game.state.cars[0].boost<1.02f); // A cooldown tap cannot extend a burst.
  for(int n=0;n<370;n++)game.step(1.f/60,game.bot(0),game.bot(1),2,0);
  assert(game.state.cars[0].cooldown==0&&game.state.cars[0].boost==0);
  game.state.cars[0].turboRequest=65535;game.step(1.f/60,0,0,0,0);assert(game.state.cars[0].boost==2);
  bad=game.state;bad.cars[0].boost=3;assert(!rally::validState(bad));
  bad=game.state;bad.cars[0].cooldown=std::numeric_limits<float>::infinity();assert(!rally::validState(bad));
  game.reset(48);assert(game.state.cars[0].boost==0&&game.state.cars[0].turboRequest==0);
  // The camera keeps its car at (60,CAR_Y), forward up, and its right side right.
  for(float angle=-3;angle<=3;angle+=.3f){
    rally::Car c;c.x=220;c.y=220;c.angle=angle;
    auto center=rally::Renderer::camera({c.x,c.y},c),front=rally::Renderer::camera({c.x+10*std::cos(angle),c.y+10*std::sin(angle)},c),right=rally::Renderer::camera({c.x-10*std::sin(angle),c.y+10*std::cos(angle)},c);
    assert(std::fabs(center.x-60)<.001f&&std::fabs(center.y-rally::CAR_Y)<.001f);
    assert(std::fabs(front.x-60)<.001f&&std::fabs(front.y-(rally::CAR_Y-10))<.001f);
    assert(std::fabs(right.x-70)<.001f&&std::fabs(right.y-rally::CAR_Y)<.001f);
  }
  // Rendering must preserve a short missed section between lit neighbours.
  game.reset(49);game.state.phase=rally::RACING;
  auto &painted=game.state.cars[0];for(auto &word:painted.coverage)word=0xffffffffu;
  painted.coverage[62/32]&=~(uint32_t(1)<<(62%32));painted.covered=rally::COVERAGE_SECTIONS-1;
  auto focus=game.track.at(game.track.length*64.5f/rally::COVERAGE_SECTIONS);
  painted.x=focus.point.x;painted.y=focus.point.y;painted.angle=std::atan2(focus.tangent.y,focus.tangent.x);
  game.state.cars[1].x=515;game.state.cars[1].y=535;
  rally::Renderer roadRenderer;roadRenderer.init(game.track);roadRenderer.world(game.state,0);
  for(int section:{62,63}){
    auto p=game.track.at(game.track.length*(section+.5f)/rally::COVERAGE_SECTIONS);
    auto screen=rally::Renderer::camera({p.point.x-p.tangent.y*4,p.point.y+p.tangent.x*4},painted);
    assert(roadRenderer.pixels[std::lround(screen.y)*rally::W+std::lround(screen.x)]==(section==62?rally::ROAD:rally::LIT_BLUE));
  }
  auto background=roadRenderer.pixels[rally::MAP_Y*rally::W+rally::MAP_X];roadRenderer.miniMap(game.state,0);
  assert(roadRenderer.pixels[rally::MAP_Y*rally::W+rally::MAP_X]==background);
  // The ribbon is wide: 18 world pixels from the centre line still collects coverage.
  game.reset(50);game.state.phase=rally::RACING;
  {auto p=game.track.at(game.track.length*80.5f/rally::COVERAGE_SECTIONS);auto &c=game.state.cars[0];c.x=p.point.x-p.tangent.y*18;c.y=p.point.y+p.tangent.x*18;c.speed=0;
  game.state.cars[1].x=515;game.state.cars[1].y=535;for(auto &u:game.state.pickups)u.respawn=rally::PICKUP_RESPAWN;game.step(1.f/60,0,0);assert(rally::coveredSection(c,80));}
  // A bot that skipped sections turns back for the nearest hole, then resumes forward.
  game.reset(51);game.state.phase=rally::RACING;game.brains[0].calm=1e6f;
  {auto &c=game.state.cars[0];for(int i=10;i<40;i++)if(i<30||i>32){c.coverage[i/32]|=uint32_t(1)<<(i%32);c.covered++;}
  auto p=game.track.at(game.track.length*40.5f/rally::COVERAGE_SECTIONS);c.x=p.point.x;c.y=p.point.y;c.angle=std::atan2(p.tangent.y,p.tangent.x);c.speed=40;
  game.state.cars[1].x=515;game.state.cars[1].y=535;for(auto &u:game.state.pickups)u.respawn=rally::PICKUP_RESPAWN;
  game.step(1.f/60,game.bot(0),0);assert(game.brains[0].target==32);
  int n=0;for(;n<900&&!(rally::coveredSection(c,30)&&rally::coveredSection(c,31)&&rally::coveredSection(c,32));n++){game.step(1.f/60,game.bot(0),0);for(auto &u:game.state.pickups)u.respawn=rally::PICKUP_RESPAWN;}
  assert(n<900);for(int k=0;k<5;k++)game.step(1.f/60,game.bot(0),0);assert(game.brains[0].target==-1);
  for(int k=0;k<300;k++)game.step(1.f/60,game.bot(0),0);
  assert(game.track.project(c.x,c.y).s>game.track.length*41/rally::COVERAGE_SECTIONS);}
  // Speed upgrades: collected on contact, stack to a cap, respawn elsewhere and are validated.
  game.reset(52);assert(rally::validState(game.state));
  for(const auto &u:game.state.pickups){assert(u.respawn==0&&u.section<rally::COVERAGE_SECTIONS);}
  game.state.phase=rally::RACING;
  {auto &c=game.state.cars[0];auto &u=game.state.pickups[0];for(int k=1;k<rally::PICKUPS;k++)game.state.pickups[k].respawn=rally::PICKUP_RESPAWN;
  game.state.cars[1].x=515;game.state.cars[1].y=535;auto at=rally::pickupPoint(game.track,u);c.x=at.x;c.y=at.y;c.speed=0;
  game.step(1.f/60,0,0);assert(c.upgrades==1&&u.respawn>0);
  bad=game.state;bad.cars[0].upgrades=rally::MAX_UPGRADES+1;assert(!rally::validState(bad));
  bad=game.state;bad.pickups[1].section=rally::COVERAGE_SECTIONS;assert(!rally::validState(bad));
  bad=game.state;bad.pickups[1].offset=100;assert(!rally::validState(bad));
  u.respawn=1;game.step(1.f/60,0,0);assert(u.respawn==0&&rally::validState(game.state));
  c.upgrades=rally::MAX_UPGRADES;at=rally::pickupPoint(game.track,u);c.x=at.x;c.y=at.y;game.step(1.f/60,0,0);assert(u.respawn==0&&c.upgrades==rally::MAX_UPGRADES);}
  {rally::Game slow,fast;slow.reset(53);fast.reset(53);slow.state.phase=fast.state.phase=rally::RACING;fast.state.cars[0].upgrades=rally::MAX_UPGRADES;
  for(int n=0;n<120;n++){for(auto *g:{&slow,&fast})for(auto &u:g->state.pickups)u.respawn=rally::PICKUP_RESPAWN;slow.step(1.f/60,0,0);fast.step(1.f/60,0,0);}
  assert(slow.state.cars[0].speed<49&&fast.state.cars[0].speed>48*1.3f);
  fast.reset(54);assert(fast.state.cars[0].upgrades==0);}
  if(argc>1){
    game.reset(46);for(int n=0;n<430;n++)game.step(1.f/60,game.bot(0),game.bot(1));
    rally::Renderer renderer;renderer.init(game.track);rally::View v;v.race=game.state;v.screen=4;v.linked=true;renderer.draw(v);
    std::ofstream f(argv[1],std::ios::binary);f<<"P6\n120 160\n255\n";
    for(auto color:renderer.pixels){char rgb[3]={char((color>>11)*255/31),char(((color>>5)&63)*255/63),char((color&31)*255/31)};f.write(rgb,3);}
  }
  std::cout<<"Coverage laps, missed sections, reverse collection, personal progress, off-road speed, collisions, finish freeze, invalid state, turbo, camera, wide ribbon, bot backtracking and speed upgrade checks passed.\n";
}
