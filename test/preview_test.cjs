// Headless gameplay test. Install @napi-rs/canvas to run outside the supplied runtime.
const fs=require('fs'),path=require('path'),vm=require('vm'),assert=require('assert');
const {createCanvas}=require(require.resolve('@napi-rs/canvas',{paths:[process.env.CODEX_PRIMARY_RUNTIME_NODE_MODULES||process.cwd()]}));
const html=fs.readFileSync(path.join(__dirname,'../web/preview.inline.html'),'utf8');
const scripts=[...html.matchAll(/<script(?:[^>]*)>([\s\S]*?)<\/script>/g)];
const art=scripts[0][1];let source=scripts[1][1];new Function(source);
// Expose state and manual rendering only in this test harness.
source=source.replace('if(now-renderAt>=32||renderAt===0)', 'if(false)').replace('    requestAnimationFrame(frame);\n  })();', '    window.inspect=()=>({phase,clock,cars,requests,paused,coveragePoints,sections});window.render=()=>{draw(0);draw(1);};\n    requestAnimationFrame(frame);\n  })();');
class Element {
  constructor(dataset={}){this.dataset=dataset;this.listeners={};this.disabled=false;this.textContent='';this.attributes={};this.tagName='BUTTON';}
  addEventListener(name,fn){this.listeners[name]=fn;} setAttribute(k,v){this.attributes[k]=v;}
  click(){if(!this.disabled)this.listeners.click?.({target:this});}setPointerCapture(){} hasPointerCapture(){return true;}
}
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
vm.runInNewContext(source,{document,window,requestAnimationFrame:fn=>next=fn,Uint16Array,JSON,Math,Number,String,console});
const tick=(n=1)=>{for(let i=0;i<n;i++){now+=1000/60;const fn=next;next=null;fn(now);}};
tick();modes[2].click();tick(650);
window.render();
if(process.argv[2])fs.writeFileSync(process.argv[2],screens[0].toBuffer('image/png'));
tick(10000);assert(status.textContent.includes('wins')||status.textContent.includes('Photo finish'));assert(pause.disabled);
console.log('Demo:',status.textContent);
modes[1].click();tick(240);turboButtons[0].click();tick(2);assert(window.inspect().cars[0].boost>1.95);tick(60);assert(window.inspect().cars[0].boost<1.05);turboButtons[0].click();tick(70);assert.equal(window.inspect().cars[0].boost,0);assert(window.inspect().cars[0].cooldown>4.7);root.listeners.keydown({key:'a',preventDefault(){}});tick(60);root.listeners.keyup({key:'a',preventDefault(){}});
pause.click();const freeze=JSON.stringify(window.inspect().cars);tick(180);assert.equal(JSON.stringify(window.inspect().cars),freeze);assert.equal(status.textContent,'Race paused.');pause.click();assert.equal(status.textContent,'Race resumed.');
modes[0].click();tick(200);buttons[1].listeners.pointerdown({pointerId:1});tick(30);buttons[1].listeners.pointerup();tick(60);
assert.equal(modes[0].attributes['aria-pressed'],'true');
window.render();
console.log('Turbo expiry, cooldown, pause freeze, keyboard steering, touch buttons, pause/resume, rematch and solo start passed.');
modes[1].click();tick(200);
const state=window.inspect();for(const c of state.cars){c.coverage.fill(0);c.covered=0;c.lap=0;}
state.cars[1].x=250;state.cars[1].y=250;state.cars[1].speed=0;
const visit=i=>{const a=state.coveragePoints[i],b=state.coveragePoints[i+1],c=state.cars[0];c.x=(a.x+b.x)/2;c.y=(a.y+b.y)/2;c.speed=0;c.angle=Math.atan2(b.y-a.y,b.x-a.x);tick();};
for(let i=0;i<state.sections;i++)if(i!==47)visit(i);
assert.equal(state.cars[0].lap,0);assert.equal(state.cars[0].covered,state.sections-1);
assert.equal(state.cars[1].covered,0);
visit(0);assert.equal(state.cars[0].lap,0);visit(47);assert.equal(state.cars[0].lap,1);assert.equal(state.cars[0].covered,0);
console.log('Preview coverage: skipped section blocks lap, repeat visits do not help, progress is personal, completing coverage resets the next lap.');
