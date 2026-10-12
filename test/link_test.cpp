#include "link.hpp"
#include <cassert>
#include <iostream>

int main(){
  using namespace rally;
  Game game;game.reset(500);game.state.phase=RACING;
  Link host;assert(host.begin());host.choose(Link::HOST);
  uint8_t mac[6]={1,2,3,4,5,6};Packet p;p.kind=JOIN;p.session=host.session;p.boot=400;
  auto receive=[&](Packet packet){receivePacket(mac,reinterpret_cast<uint8_t*>(&packet),sizeof(packet));host.pump(game,0);};
  receive(p);assert(host.paired);
  p.kind=CONTROL;p.race.raceId=500;p.sequence=1;p.turboRequest=1;receive(p);
  assert(host.remoteTurbo==1);game.step(1.f/60,0,0,0,host.remoteTurbo);assert(game.state.cars[1].boost==2);
  for(int n=0;n<60;n++){p.sequence++;receive(p);game.step(1.f/60,game.bot(0),game.bot(1),0,host.remoteTurbo);}
  assert(game.state.cars[1].boost<1.02f); // Retransmission never extends a burst.
  p.sequence=2;p.turboRequest=9;receive(p);assert(host.remoteTurbo==1); // Reordered packet.
  p.sequence=100;p.race.raceId=499;receive(p);assert(host.remoteTurbo==1); // Previous race.
  p.sequence=101;p.race.raceId=500;p.turboRequest=2; // Simulate losing this transmission.
  p.sequence=102;receive(p);assert(host.remoteTurbo==2); // The next repeated input arrives.
  game.step(1.f/60,0,0,0,host.remoteTurbo);assert(game.state.cars[1].turboRequest==2&&game.state.cars[1].boost<1.02f);
  p.sequence=103;p.version=2;p.turboRequest=7;receive(p);assert(host.remoteTurbo==2);
  fakeNow+=601;assert(!host.fresh());
  game.reset(501);host.pump(game,0);assert(host.localTurbo==0&&host.remoteTurbo==0);
  p.version=PROTOCOL;p.sequence=104;p.race.raceId=500;receive(p);assert(host.remoteTurbo==0);
  // Rebooted guest resumes at the acknowledged counter, without phantom turbo.
  game.state.cars[1].turboRequest=12;p.kind=JOIN;p.boot=401;receive(p);assert(host.remoteTurbo==12);
  host.choose(Link::GUEST);p=Packet{};p.kind=HELLO;p.boot=900;p.session=800;receive(p);
  Game authoritative;authoritative.reset(600);authoritative.state.phase=RACING;authoritative.state.cars[1].turboRequest=12;
  p.kind=WELCOME;p.sequence=1;p.race=authoritative.state;receive(p);assert(host.paired&&host.localTurbo==12);
  host.localTurbo=13;p.kind=SNAPSHOT;p.sequence=2;receive(p);assert(host.localTurbo==13); // Unacknowledged local tap survives a snapshot.
  game.step(1.f/60,0,0,host.remoteTurbo,host.localTurbo);assert(game.state.cars[1].boost==2);
  authoritative.state.cars[1].turboRequest=13;authoritative.state.cars[1].boost=1.7f;authoritative.state.cars[1].cooldown=6.7f;
  authoritative.state.cars[0].coverage[2]=uint32_t(1)<<5;authoritative.state.cars[0].covered=1;
  authoritative.state.cars[1].coverage[5]=uint32_t(1)<<31;authoritative.state.cars[1].covered=1;
  p.sequence=3;p.race=authoritative.state;receive(p);
  assert(coveredSection(game.state.cars[0],69)&&coveredSection(game.state.cars[1],191));
  game.step(1.f/60,0,0,host.remoteTurbo,host.localTurbo);assert(game.state.cars[1].boost<1.7f);
  p.sequence=4;p.race.cars[0].covered=192;receive(p);assert(game.state.cars[0].covered!=192); // Reject inconsistent coverage data.
  authoritative.reset(601);p.sequence=5;p.race=authoritative.state;receive(p);assert(host.localTurbo==0&&host.remoteTurbo==0);
  assert(game.state.cars[0].covered==0&&game.state.cars[1].covered==0);
  std::cout<<"Link v3: turbo input handling, old protocol rejection, timeout, race reset, reboot, coverage snapshots and invalid-coverage rejection passed. Packet="<<sizeof(Packet)<<" bytes.\n";
  delete rxQueue;rxQueue=nullptr;
}
