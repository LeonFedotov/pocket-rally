#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>

namespace rally {
constexpr int W=120,H=160,LAPS=3,COVERAGE_SECTIONS=192,COVERAGE_WORDS=COVERAGE_SECTIONS/32,WORLD_W=520,WORLD_H=540;
constexpr float ROAD_HALF=12,TURBO_TIME=2,TURBO_RECOVERY=5;
constexpr float PI_F=3.14159265358979323846f;
struct Vec { float x,y; };
inline float clamp(float n,float a,float b){return std::max(a,std::min(b,n));}
inline float wrap(float a){while(a>PI_F)a-=2*PI_F;while(a<-PI_F)a+=2*PI_F;return a;}
struct Projection { float distance,s; Vec point,tangent; };

// Closed world-space circuit. Cameras follow each car independently.
class Track {
 public:
  Vec points[97]{}; float lengths[97]{}; int count=0; float length=0;
  Track(){
    const Vec knots[]={{180,70},{310,70},{430,130},{420,255},{325,300},{400,405},{300,475},{150,450},{70,350},{145,260},{70,160},{110,90}};
    for(int i=0;i<12;i++)for(int j=0;j<8;j++){
      Vec a=knots[(i+11)%12],b=knots[i],c=knots[(i+1)%12],d=knots[(i+2)%12];float t=j/8.f,t2=t*t,t3=t2*t;
      add(.5f*(2*b.x+(-a.x+c.x)*t+(2*a.x-5*b.x+4*c.x-d.x)*t2+(-a.x+3*b.x-3*c.x+d.x)*t3),.5f*(2*b.y+(-a.y+c.y)*t+(2*a.y-5*b.y+4*c.y-d.y)*t2+(-a.y+3*b.y-3*c.y+d.y)*t3));
    }
    add(points[0].x,points[0].y);
    for(int i=1;i<count;i++){float dx=points[i].x-points[i-1].x,dy=points[i].y-points[i-1].y;length+=std::sqrt(dx*dx+dy*dy);lengths[i]=length;}
  }
  Projection project(float x,float y) const {
    Projection out{1e12f,0,{0,0},{1,0}};
    for(int i=0;i<count-1;i++){
      Vec a=points[i],b=points[i+1];float dx=b.x-a.x,dy=b.y-a.y,l2=dx*dx+dy*dy;
      float t=clamp(((x-a.x)*dx+(y-a.y)*dy)/l2,0,1),px=a.x+dx*t,py=a.y+dy*t;
      float d2=(x-px)*(x-px)+(y-py)*(y-py);
      if(d2<out.distance){float l=lengths[i+1]-lengths[i];out={d2,lengths[i]+t*l,{px,py},{dx/l,dy/l}};}
    }
    out.distance=std::sqrt(out.distance);return out;
  }
  Projection at(float s) const {
    while(s<0)s+=length;
    while(s>=length)s-=length;
    for(int i=0;i<count-1;i++)if(s<=lengths[i+1]){
      float l=lengths[i+1]-lengths[i],t=(s-lengths[i])/l;
      Vec a=points[i],b=points[i+1];return {0,s,{a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t},{(b.x-a.x)/l,(b.y-a.y)/l}};
    }
    return {0,0,points[0],{1,0}};
  }
 private:
  void add(float x,float y){points[count++]={x,y};}
};

enum Phase : uint8_t { LOBBY=0, COUNTDOWN=1, RACING=2, FINISHED=3 };
struct Car {
  float x=0,y=0,angle=0,speed=0,finish=0,boost=0,cooldown=0;
  uint32_t coverage[COVERAGE_WORDS]{};
  uint16_t covered=0;uint8_t lap=0,reserved=0;uint16_t turboRequest=0,padding=0;
};
inline bool coveredSection(const Car &car,int section){return (car.coverage[section/32]&(uint32_t(1)<<(section%32)))!=0;}
inline int coveragePercent(const Car &car){return car.covered*100/COVERAGE_SECTIONS;}
struct State {
  Car cars[2]; float clock=0,countdown=3; uint32_t tick=0,raceId=0;
  uint8_t phase=LOBBY,winner=255; uint16_t padding=0;
};
static_assert(sizeof(State)<160,"State must fit an ESP-NOW v1 packet");

class Game {
 public:
  Track track; State state;
  Game(){reset(0);state.phase=LOBBY;}
  void reset(uint32_t id){state=State{};state.raceId=id;state.phase=COUNTDOWN;auto p=track.at(-10);for(int i=0;i<2;i++){auto &c=state.cars[i];float offset=i?5:-5;c.x=p.point.x-p.tangent.y*offset;c.y=p.point.y+p.tangent.x*offset;c.angle=std::atan2(p.tangent.y,p.tangent.x);}}
  float bot(int i) const {
    const auto &c=state.cars[i];auto p=track.project(c.x,c.y);auto target=track.at(p.s+16+c.speed*.10f);
    float angle=std::atan2(target.point.y-c.y,target.point.x-c.x);
    return clamp(wrap(angle-c.angle)*2.1f,-1,1);
  }
  bool botTurbo(int i) const {
    const auto &c=state.cars[i];auto p=track.project(c.x,c.y),a=track.at(p.s+35),b=track.at(p.s+90);
    float turn=wrap(std::atan2(b.tangent.y,b.tangent.x)-std::atan2(a.tangent.y,a.tangent.x));
    return state.phase==RACING&&c.cooldown==0&&p.distance<4&&std::fabs(bot(i))<.15f&&std::fabs(turn)<.35f;
  }
  void step(float dt,float left,float right,uint16_t turbo0=0,uint16_t turbo1=0){
    if(state.phase==LOBBY||state.phase==FINISHED)return;
    dt=clamp(dt,0,.034f);state.tick++;
    if(state.phase==COUNTDOWN){state.countdown-=dt;if(state.countdown<=0){state.countdown=0;state.phase=RACING;}return;}
    state.clock+=dt;
    turbo(0,turbo0,dt);turbo(1,turbo1,dt);move(0,left,dt);move(1,right,dt);collide();
    for(int i=0;i<2;i++)recordCoverage(i);
    if(state.cars[0].lap>=LAPS||state.cars[1].lap>=LAPS){
      state.phase=FINISHED;
      if(state.cars[0].lap>=LAPS&&state.cars[1].lap>=LAPS){
        const float a=state.cars[0].finish,b=state.cars[1].finish;
        state.winner=std::fabs(a-b)<.0001f?2:(a<b?0:1);
      }else state.winner=state.cars[0].lap>=LAPS?0:1;
    }
  }
 private:
  void turbo(int i,uint16_t request,float dt){
    auto &c=state.cars[i];c.boost=std::max(0.f,c.boost-dt);c.cooldown=std::max(0.f,c.cooldown-dt);
    // A repeated packet acknowledges one tap, without retriggering it.
    if(request!=c.turboRequest){c.turboRequest=request;if(c.cooldown==0){c.boost=TURBO_TIME;c.cooldown=TURBO_TIME+TURBO_RECOVERY;}}
  }
  void move(int i,float steer,float dt){
    auto &c=state.cars[i];steer=clamp(steer,-1,1);
    float target=(c.boost>0?78.f:48.f)-std::fabs(steer)*8.f;
    c.speed+=(target-c.speed)*std::min(1.f,dt*(c.boost>0?7.f:3.5f));
    c.angle=wrap(c.angle+steer*2.35f*dt*(.65f+.35f*c.speed/48.f));
    c.x=clamp(c.x+std::cos(c.angle)*c.speed*dt,5,WORLD_W-5);
    c.y=clamp(c.y+std::sin(c.angle)*c.speed*dt,5,WORLD_H-5);
  }
  void collide(){
    auto &a=state.cars[0];auto &b=state.cars[1];float dx=b.x-a.x,dy=b.y-a.y,d2=dx*dx+dy*dy;
    if(d2>=49)return;
    float distance=std::sqrt(d2),d=distance;
    if(d<.001f){dx=0;dy=1;d=1;}
    float nx=dx/d,ny=dy/d,shift=(7-distance)*.5f;
    a.x=clamp(a.x-nx*shift,5,WORLD_W-5);a.y=clamp(a.y-ny*shift,5,WORLD_H-5);
    b.x=clamp(b.x+nx*shift,5,WORLD_W-5);b.y=clamp(b.y+ny*shift,5,WORLD_H-5);
    float closing=(std::cos(a.angle)*a.speed-std::cos(b.angle)*b.speed)*nx+(std::sin(a.angle)*a.speed-std::sin(b.angle)*b.speed)*ny;
    if(closing>0){a.speed=std::max(5.f,a.speed-closing*.3f);b.speed=std::max(5.f,b.speed-closing*.1f);}
  }
  void recordCoverage(int i){
    auto &c=state.cars[i];if(c.lap>=LAPS)return;auto p=track.project(c.x,c.y);
    if(p.distance>ROAD_HALF)return;
    int section=std::min(COVERAGE_SECTIONS-1,int(p.s*COVERAGE_SECTIONS/track.length));
    if(coveredSection(c,section))return;
    c.coverage[section/32]|=uint32_t(1)<<(section%32);c.covered++;
    if(c.covered==COVERAGE_SECTIONS){
      c.lap++;
      if(c.lap==LAPS)c.finish=state.clock;
      else {std::memset(c.coverage,0,sizeof(c.coverage));c.covered=0;}
    }
  }
};
inline bool validState(const State &s){
  if(s.phase>FINISHED||(s.winner>2&&s.winner!=255)||!std::isfinite(s.clock)||!std::isfinite(s.countdown)||s.clock<0||s.clock>100000||s.countdown<0||s.countdown>3.1f)return false;
  for(const auto &c:s.cars){
    if(!std::isfinite(c.x)||!std::isfinite(c.y)||!std::isfinite(c.angle)||!std::isfinite(c.speed)||!std::isfinite(c.finish)||!std::isfinite(c.boost)||!std::isfinite(c.cooldown)||c.x<0||c.x>WORLD_W||c.y<0||c.y>WORLD_H||std::fabs(c.angle)>3.2f||c.speed<0||c.speed>100||c.lap>LAPS||c.covered>COVERAGE_SECTIONS||c.boost<0||c.boost>TURBO_TIME||c.cooldown<0||c.cooldown>TURBO_TIME+TURBO_RECOVERY||c.boost>c.cooldown)return false;
    int count=0;for(auto word:c.coverage)for(;word;word&=word-1)count++;
    if(count!=c.covered||(c.lap==LAPS&&c.covered!=COVERAGE_SECTIONS))return false;
  }
  return true;
}
}
