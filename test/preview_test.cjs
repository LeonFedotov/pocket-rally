// Headless gameplay test. Install @napi-rs/canvas to run outside the supplied runtime.
const fs=require('fs'),path=require('path'),vm=require('vm'),assert=require('assert');
const {createCanvas}=require(require.resolve('@napi-rs/canvas',{paths:[process.env.CODEX_PRIMARY_RUNTIME_NODE_MODULES||process.cwd()]}));
const html=fs.readFileSync(path.join(__dirname,'../web/preview.inline.html'),'utf8');
const scripts=[...html.matchAll(/<script(?:[^>]*)>([\s\S]*?)<\/script>/g)];
const art=scripts[0][1],rules=JSON.parse(art).rules;let source=scripts[1][1];new Function(source);
assert(!html.includes('tap bottom centre'),'help text still mentions the old bottom turbo button');
// Expose state and manual rendering only in this test harness.
source=source.replace('if(now-renderAt>=32||renderAt===0)', 'if(false)').replace('    requestAnimationFrame(frame);\n  })();', '    window.inspect=()=>({phase,clock,cars,requests,paused,coveragePoints,sections,pickups,brains,steer,length,reset,pickupPoint,project,at,step});window.render=()=>{draw(0);draw(1);};\n    requestAnimationFrame(frame);\n  })();');
class Element {
  constructor(dataset={}){this.dataset=dataset;this.listeners={};this.disabled=false;this.textContent='';this.attributes={};this.tagName='BUTTON';}
  addEventListener(name,fn){this.listeners[name]=fn;} setAttribute(k,v){this.attributes[k]=v;}
  click(){if(!this.disabled)this.listeners.click?.({target:this});}setPointerCapture(){} hasPointerCapture(){return true;}
}
// Canvases are displayed at 2x (240x320 CSS pixels), the same as the physical panel.
const screens=[createCanvas(120,160),createCanvas(120,160)];
screens.forEach(c=>{c.listeners={};c.addEventListener=(k,v)=>c.listeners[k]=v;c.setPointerCapture=()=>{};c.hasPointerCapture=()=>true;c.getBoundingClientRect=()=>({left:0,top:0,width:240,height:320});});
const status=new Element(),pause=new Element(),modes=['solo','duo','demo'].map(mode=>new Element({mode}));
const buttons=['0,-1','0,1','1,-1','1,1'].map(steer=>new Element({steer}));
const turboButtons=[0,1].map(turbo=>new Element({turbo:String(turbo)}));
const root=new Element();root.isConnected=true;
root.querySelectorAll=s=>({'canvas':screens,'[data-mode]':modes,'[data-steer]':buttons,'[data-turbo]':turboButtons}[s]);
root.querySelector=s=>({'[data-art]':{textContent:art},'[role="status"]':status,'[data-pause]':pause}[s]);
let next,now=0;
const document={hidden:false,getElementById:id=>{assert.equal(id,'pocket-rally-preview');return root;},addEventListener(){}};
const window={addEventListener(){}};
// Every race starts from Math.random; pin it to race id 2 so the demo finish time is reproducible.
const seeded=Object.create(Math);seeded.random=()=>2/4294967296;
vm.runInNewContext(source,{document,window,requestAnimationFrame:fn=>next=fn,Uint16Array,JSON,Math:seeded,Number,String,console});
const tick=(n=1)=>{for(let i=0;i<n;i++){now+=1000/60;const fn=next;next=null;fn(now);}};
const holdPickups=()=>{for(const u of window.inspect().pickups)u.respawn=rules.pickupRespawn;};
const pointer=(i,type,x,y)=>screens[i].listeners[type]({pointerId:1,clientX:x,clientY:y});
// Seeding mirrors Game::reset bit for bit (reference values printed by the C++ game.hpp).
{const expected={0:{pickups:[[102,-9],[19,-6],[62,-10]],calm:[3.906064,3.632588]},42:{pickups:[[74,3],[152,-5],[118,12]],calm:[3.369527,2.216562]},4000000000:{pickups:[[46,3],[29,4],[113,2]],calm:[4.503067,4.048397]}};
for(const [id,e] of Object.entries(expected)){window.inspect().reset(Number(id));const s=window.inspect();assert.deepEqual(s.pickups.map(u=>[u.section,u.offset]),e.pickups);s.brains.forEach((b,i)=>assert(Math.abs(b.calm-e.calm[i])<1e-5));assert(s.pickups.every(u=>u.respawn===0));}}
// Two-bot trajectory matches the C++ Game (mistakes, backtracking, pickups, bot turbo) for race id 1.
{modes[2].click();const s=window.inspect();s.reset(1);while(window.inspect().phase===1)s.step(1/60);
const expected={540:[371.575,278.785,467.055,204.622,0,1],1000:[347.601,458.859,342.967,453.613,0,2]};
for(let n=0;n<=1000;n++){s.step(1/60);const e=expected[n];if(e){const [a,b]=window.inspect().cars;[a.x,a.y,b.x,b.y].forEach((v,k)=>assert(Math.abs(v-e[k])<.01,`tick ${n}: ${v} vs ${e[k]}`));assert.deepEqual([a.upgrades,b.upgrades],e.slice(4));}}
console.log('Bot race trajectory matches the firmware game for 1000 ticks.');}
tick();modes[2].click();tick(650);
{const s=window.inspect();assert.equal(s.pickups.length,rules.pickups);for(const u of s.pickups){assert(u.section>=0&&u.section<s.sections&&Math.abs(u.offset)<=rules.pickupSpread);}}
window.render();
if(process.argv[2]){
  // Both screens side by side, enlarged 2x with nearest-neighbour like the firmware's panel push.
  const out=createCanvas(500,320),ctx=out.getContext('2d');ctx.imageSmoothingEnabled=false;ctx.fillStyle='#211e30';ctx.fillRect(0,0,500,320);
  screens.forEach((c,i)=>ctx.drawImage(c,i*260,0,240,320));fs.writeFileSync(process.argv[2],out.toBuffer('image/png'));
}
tick(10000);assert(status.textContent.includes('wins')||status.textContent.includes('Photo finish'));assert(pause.disabled);
for(const c of window.inspect().cars)assert(c.speed<=rules.maxSpeed&&c.upgrades<=rules.maxUpgrades);
console.log('Demo:',status.textContent,'· upgrades',window.inspect().cars.map(c=>c.upgrades).join('/'));
modes[1].click();tick(240);turboButtons[0].click();tick(2);assert(window.inspect().cars[0].boost>1.95);tick(60);assert(window.inspect().cars[0].boost<1.05);turboButtons[0].click();tick(70);assert.equal(window.inspect().cars[0].boost,0);assert(window.inspect().cars[0].cooldown>4.7);root.listeners.keydown({key:'a',preventDefault(){}});tick(60);root.listeners.keyup({key:'a',preventDefault(){}});
pause.click();const freeze=JSON.stringify(window.inspect().cars);tick(180);assert.equal(JSON.stringify(window.inspect().cars),freeze);assert.equal(status.textContent,'Race paused.');pause.click();assert.equal(status.textContent,'Race resumed.');
modes[0].click();tick(200);buttons[1].listeners.pointerdown({pointerId:1});assert.equal(window.inspect().steer[0],1);tick(30);buttons[1].listeners.pointerup();assert.equal(window.inspect().steer[0],0);tick(60);
assert.equal(modes[0].attributes['aria-pressed'],'true');
window.render();
console.log('Turbo expiry, cooldown, pause freeze, keyboard steering, full-lock steer buttons, pause/resume, rematch and solo start passed.');
// Canvas touch: proportional steering from X with an 8px dead zone (in 240-pixel panel units), centred on release.
modes[1].click();tick(200);
{const s=window.inspect();
pointer(1,'pointerdown',232,200);assert.equal(s.steer[1],1);assert.equal(s.steer[0],0);
pointer(1,'pointermove',124,200);assert.equal(s.steer[1],0);pointer(1,'pointermove',115,200);assert.equal(s.steer[1],0);
pointer(1,'pointermove',176,200);assert.equal(s.steer[1],.5);pointer(1,'pointermove',20,200);assert.equal(s.steer[1],-.95);pointer(1,'pointermove',0,200);assert.equal(s.steer[1],-1);
tick(30);assert(s.cars[1].angle!==undefined);pointer(1,'pointerup');assert.equal(s.steer[1],0);
// Turbo strip: the top 28 of 160 rows; pressing below it does nothing, sliding in fires once per entry.
const before=s.requests[1];pointer(1,'pointerdown',120,300);assert.equal(s.requests[1],before);
pointer(1,'pointermove',120,40);assert.equal(s.requests[1],before+1);assert.equal(s.steer[1],0);tick(2);assert(s.cars[1].boost>1.9);
pointer(1,'pointermove',60,20);pointer(1,'pointermove',200,50);assert.equal(s.requests[1],before+1);assert(s.steer[1]>0);
pointer(1,'pointermove',120,100);pointer(1,'pointermove',120,10);assert.equal(s.requests[1],before+1,'cooldown blocks a second burst');
pointer(1,'pointerup');assert.equal(s.steer[1],0);
tick(7*60+5);assert.equal(s.cars[1].cooldown,0);pointer(1,'pointerdown',120,30);assert.equal(s.requests[1],before+2);pointer(1,'pointerup');}
console.log('Canvas steering (dead zone, proportional, release centres) and turbo strip (press, slide-in once per entry) passed.');
// Speed upgrades: collected on contact, capped, respawn, and each raises target speed.
modes[1].click();tick(200);
{const s=window.inspect(),[c,other]=s.cars,u=s.pickups[0];s.pickups.slice(1).forEach(p=>p.respawn=rules.pickupRespawn);other.x=515;other.y=535;other.speed=0;
let p=s.pickupPoint(u);c.x=p.x;c.y=p.y;c.speed=0;tick();assert.equal(c.upgrades,1);assert(u.respawn>0);
u.respawn=1;tick();assert.equal(u.respawn,0);
c.upgrades=rules.maxUpgrades;p=s.pickupPoint(u);c.x=p.x;c.y=p.y;tick();assert.equal(u.respawn,0);assert.equal(c.upgrades,rules.maxUpgrades);}
const cruise=upgrades=>{modes[1].click();tick(200);const s=window.inspect();s.cars[0].upgrades=upgrades;s.cars[1].x=515;s.cars[1].y=535;for(let n=0;n<120;n++){holdPickups();tick();}return s.cars[0].speed;};
{const slow=cruise(0),fast=cruise(rules.maxUpgrades);assert(slow<49&&fast>48*(1+rules.upgradeGain*rules.maxUpgrades)-1&&fast>48*1.3,`speeds ${slow} ${fast}`);
console.log('Speed upgrades: pickups exist, collect, cap, respawn; cruise',slow.toFixed(1),'→',fast.toFixed(1),'with',rules.maxUpgrades,'upgrades.');}
// Bot backtracking: a bot that skipped sections reverses to the nearest hole, then resumes forward.
modes[2].click();tick(200);
{const s=window.inspect(),c=s.cars[0],b=s.brains[0];b.calm=1e6;b.mistake=0;b.target=-1;b.rest=0;
const o=s.cars[1];o.x=515;o.y=535;s.brains[1].mistake=1e9;s.brains[1].bias=1;
c.coverage.fill(0);c.covered=0;c.lap=0;for(let i=10;i<40;i++)if(i<30||i>32){c.coverage[i>>>5]|=1<<(i&31);c.covered++;}
const p=s.at(s.length*40.5/s.sections);c.x=p.x;c.y=p.y;c.angle=Math.atan2(p.ty,p.tx);c.speed=40;holdPickups();
tick();assert.equal(b.target,32);
const done=i=>(c.coverage[i>>>5]&(1<<(i&31)))!==0;let n=0;for(;n<900&&!(done(30)&&done(31)&&done(32));n++){tick();holdPickups();}
assert(n<900,'bot never filled the hole');tick(5);assert.equal(b.target,-1);tick(300);assert(s.project(c.x,c.y).s>s.length*41/s.sections);
console.log('Bot backtracking filled the hole in',n,'ticks, then resumed forward.');}
modes[1].click();tick(200);
const state=window.inspect();for(const c of state.cars){c.coverage.fill(0);c.covered=0;c.lap=0;}
const visit=i=>{const a=state.coveragePoints[i],b=state.coveragePoints[i+1],c=state.cars[0];c.x=(a.x+b.x)/2;c.y=(a.y+b.y)/2;c.speed=0;c.angle=Math.atan2(b.y-a.y,b.x-a.x);state.cars[1].x=515;state.cars[1].y=535;state.cars[1].speed=0;tick();};
for(let i=0;i<state.sections;i++)if(i!==47)visit(i);
assert.equal(state.cars[0].lap,0);assert.equal(state.cars[0].covered,state.sections-1);
assert.equal(state.cars[1].covered,0);
visit(0);assert.equal(state.cars[0].lap,0);visit(47);assert.equal(state.cars[0].lap,1);assert.equal(state.cars[0].covered,0);
console.log('Preview coverage: skipped section blocks lap, repeat visits do not help, progress is personal, completing coverage resets the next lap.');
