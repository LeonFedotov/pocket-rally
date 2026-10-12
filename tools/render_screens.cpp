#include "render.hpp"
#include <fstream>
#include <string>
// Writes two PPM images from the actual renderer at the same simulated instant.
int main(int argc,char **argv){
  if(argc!=2)return 1;
  rally::Game game;game.reset(1);
  while(game.state.phase==rally::COUNTDOWN)game.step(1.f/60,0,0);
  for(int n=0;n<540;n++)game.step(1.f/60,game.bot(0),game.bot(1),n>=480?1:0,0);
  rally::Renderer renderer;renderer.init(game.track);
  for(int i=0;i<2;i++){
    rally::View v;v.race=game.state;v.local=i;v.screen=4;v.linked=true;renderer.draw(v);
    std::ofstream f(std::string(argv[1])+"/player-"+std::to_string(i+1)+".ppm",std::ios::binary);if(!f)return 2;
    f<<"P6\n120 160\n255\n";
    for(auto c:renderer.pixels){char rgb[3]={char((c>>11)*255/31),char(((c>>5)&63)*255/63),char((c&31)*255/31)};f.write(rgb,3);}
  }
}
