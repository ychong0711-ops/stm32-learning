#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
build_learning5.py — 가상 레지스터 시뮬레이터 + 인터럽트 우선순위 계산기 (멱등)
  1) 🧪 시뮬레이터 탭: GPIO(ODR/MODER) + CAN 프레임(vTask_CAN_Rx 경로 재현)
  2) 🎚️ 인터럽트 탭: NVIC 우선순위 계산 + FreeRTOS FromISR 안전성 판정
핵심 계산은 순수 함수(gpioComputeRegs/canParseFrame/canBitTime/irqCompute)로 분리.
build_learning.py + 3 + 4 실행 후 실행한다.
"""
HTML = '/home/user/learning_program.html'
html = open(HTML, encoding='utf-8').read()

# ===========================================================================
# A. 내비게이션 버튼 추가
# ===========================================================================
if 'data-s="sim"' not in html:
    html = html.replace(
        '    <button data-s="missions">🎯 코딩 과제</button>',
        '    <button data-s="missions">🎯 코딩 과제</button>\n    <button data-s="sim">🧪 시뮬레이터</button>\n    <button data-s="irq">🎚️ 인터럽트</button>'
    )

# ===========================================================================
# B. 섹션 HTML 주입
# ===========================================================================
if 'section id="sim"' not in html:
    sections = '''<!-- ============ SIM ============ -->
<section id="sim">
  <h1>🧪 가상 레지스터 시뮬레이터</h1>
  <p class="dim">브라우저에서 STM32 레지스터와 CAN 버스를 조작해 봅니다. <b>bsp_gpio.c</b> 와 <b>bsp_can.c</b> 가 내부에서 하는 일을 그대로 재현합니다. (교육용 시뮬레이션 — 실제 하드웨어 동작과 다를 수 있음)</p>
  <div class="grid">
    <div class="card">
      <h2>🟢 GPIO — ODR / MODER 레지스터</h2>
      <p class="dim">bsp_gpio.c 의 논리핀→물리핀 매핑 (PA4·PA6·PA7·PB13). 버튼 조작 시 레지스터 비트가 갱신됩니다.</p>
      <div id="gpioPins"></div>
      <div id="gpioRegs"></div>
      <div id="gpioLog" class="dim" style="font:12px var(--mono);margin-top:8px"></div>
    </div>
    <div class="card">
      <h2>🔴 CAN — 프레임 송신 & 수신 경로 재현</h2>
      <p class="dim">vTask_CAN_Rx / prvConvertCanToActuatorCmd 로직 재현 — <b>0x123</b> 긴급 · <b>0x200</b> 스로틀 · <b>0x201</b> 팬</p>
      <div class="dim" style="margin:6px 0">ID(hex):
        <input id="canId" value="0x200" style="width:80px;background:#0b1120;color:#d6e4ff;border:1px solid var(--line);border-radius:7px;padding:6px;font:13px var(--mono)">
        DLC: <select id="canDlc" style="background:#0b1120;color:#d6e4ff;border:1px solid var(--line);border-radius:7px;padding:6px"></select>
      </div>
      <div class="dim" style="margin:6px 0">데이터(hex): <span id="canDataIn"></span></div>
      <div>
        <button class="act" onclick="canSend()">📤 전송</button>
        <button class="ghost" onclick="canPreset('emg')">긴급정지 0x123</button>
        <button class="ghost" onclick="canPreset('thr')">스로틀 50%</button>
        <button class="ghost" onclick="canPreset('fan')">팬 ON</button>
      </div>
      <div id="canOut"></div>
    </div>
  </div>
</section>

<!-- ============ IRQ ============ -->
<section id="irq">
  <h1>🎚️ 인터럽트 우선순위 계산기 (NVIC)</h1>
  <p class="dim">STM32F4 (configPRIO_BITS=4). <b>우선순위 그룹 → 선점/서브 비트 분할 → NVIC 레지스터 값 → FreeRTOS FromISR 안전성</b>을 한 번에 계산합니다.</p>
  <div class="grid">
    <div class="card">
      <h2>우선순위 계산</h2>
      <div class="dim">우선순위 그룹:
        <select id="irqGroup" style="background:#0b1120;color:#d6e4ff;border:1px solid var(--line);border-radius:7px;padding:6px">
          <option value="0">0 (선점0+서브4)</option><option value="1">1 (선점1+서브3)</option>
          <option value="2">2 (선점2+서브2)</option><option value="3">3 (선점3+서브1)</option>
          <option value="4" selected>4 (선점4+서브0)</option>
        </select>
      </div>
      <div class="dim" style="margin:6px 0">선점 우선순위: <input id="irqPre" value="5" style="width:60px;background:#0b1120;color:#d6e4ff;border:1px solid var(--line);border-radius:7px;padding:6px">
        서브 우선순위: <input id="irqSub" value="0" style="width:60px;background:#0b1120;color:#d6e4ff;border:1px solid var(--line);border-radius:7px;padding:6px"></div>
      <div>
        <button class="act" onclick="irqCalc()">계산</button>
        <button class="ghost" onclick="irqPreset(4,5,0)">우리 코드 (CAN1_RX0)</button>
        <button class="ghost" onclick="irqPreset(4,4,0)">경계 아래 (위험 예)</button>
      </div>
      <div id="irqOut" style="margin-top:10px"></div>
    </div>
    <div class="card">
      <h2>FreeRTOS FromISR 안전성</h2>
      <div class="dim">configPRIO_BITS: <input id="irqBits" value="4" style="width:50px;background:#0b1120;color:#d6e4ff;border:1px solid var(--line);border-radius:7px;padding:6px">
        configLIBRARY_MAX_SYSCALL: <input id="irqLibMax" value="5" style="width:50px;background:#0b1120;color:#d6e4ff;border:1px solid var(--line);border-radius:7px;padding:6px"></div>
      <div id="irqFreertos" style="margin-top:10px"></div>
    </div>
    <div class="card">
      <h2>그룹별 비트 분할표</h2>
      <div id="irqTable"></div>
    </div>
  </div>
</section>

'''
    html = html.replace('<div class="foot">', sections + '<div class="foot">')

# 홈 소개 카드에 항목 추가
html = html.replace(
    '<li><b>임베디드 계산기</b> — 타이머·CAN·IWDG·ADC·UART·PWM 계산 체험</li>',
    '<li><b>임베디드 계산기</b> — 타이머·CAN·IWDG·ADC·UART·PWM 계산 체험</li>\n        <li><b>시뮬레이터</b> — GPIO 레지스터·CAN 프레임을 브라우저에서 조작</li>\n        <li><b>인터럽트 계산기</b> — NVIC 우선순위 + FreeRTOS FromISR 안전성 판정</li>'
)

# ===========================================================================
# C. JS 주입
# ===========================================================================
JS_BLOCK = r'''

/* ============================================================
   가상 레지스터 시뮬레이터 (개선 — 순수 함수 + DOM 래퍼)
============================================================ */
/* ---- GPIO 순수 코어: 핀 상태 → 레지스터 값 ---- */
function gpioComputeRegs(pinStates){
  const regs={A:{MODER:0,ODR:0,IDR:0},B:{MODER:0,ODR:0,IDR:0}};
  pinStates.forEach(p=>{
    const R=regs[p.port];
    const modeBits=(p.mode==='out')?1:0;      // 00 입력, 01 출력
    R.MODER |= (modeBits << (2*p.pin));
    if(p.mode==='out'){ if(p.level) R.ODR |= (1<<p.pin); }
    else { if(p.level) R.IDR |= (1<<p.pin); }
  });
  return regs;
}

const gpinPins=[
  {logical:13, port:'B', pin:13, name:'FAN (팬 제어)', mode:'out', level:0},
  {logical:22, port:'A', pin:6,  name:'LED_ERR (오류)', mode:'out', level:0},
  {logical:20, port:'A', pin:4,  name:'MEAS_JITTER', mode:'out', level:0},
  {logical:21, port:'A', pin:7,  name:'MEAS_E2E', mode:'out', level:0},
];
const gpiolog=[];

function gpioFind(logical){ return gpinPins.find(p=>p.logical===logical); }
function gpioWrite(logical,v){
  const p=gpioFind(logical); if(!p) return;
  p.mode='out'; p.level=v?1:0;
  gpiolog.push('GPIO_Write(GPIO'+p.port+', GPIO_PIN_'+p.pin+', '+(v?'SET':'RESET')+');  → ODR bit'+p.pin+'='+p.level);
  gpioRender();
}
function gpioToggle(logical){
  const p=gpioFind(logical); if(!p) return;
  p.mode='out'; p.level=1-p.level;
  gpiolog.push('GPIO_TogglePin(GPIO'+p.port+', GPIO_PIN_'+p.pin+');  → ODR bit'+p.pin+'='+p.level);
  gpioRender();
}
function gpioSetMode(logical,m){
  const p=gpioFind(logical); if(!p) return;
  p.mode=m;
  gpiolog.push('MODER['+p.pin+'] = '+(m==='out'?'01 (출력)':'00 (입력)'));
  gpioRender();
}
function gpioRender(){
  const regs=gpioComputeRegs(gpinPins);
  let ph='';
  gpinPins.forEach(p=>{
    const on=(p.mode==='out')?p.level:0;
    ph+='<div class="card" style="margin:6px 0;padding:10px">'+
      '<b>P'+p.port+p.pin+'</b> ('+p.name+') <span class="dim">논리ID '+p.logical+'</span>'+
      '<div style="display:flex;align-items:center;gap:8px;margin:6px 0">'+
      '<span style="width:15px;height:15px;border-radius:50%;background:'+(on?'#3ddc84':'#3a4a6b')+';display:inline-block;box-shadow:0 0 8px '+(on?'#3ddc84':'transparent')+'"></span>'+
      '<select onchange="gpioSetMode('+p.logical+',this.value)" style="background:#0b1120;color:#d6e4ff;border:1px solid var(--line);border-radius:7px;padding:4px">'+
      '<option value="out"'+(p.mode==='out'?' selected':'')+'>출력</option>'+
      '<option value="in"'+(p.mode==='in'?' selected':'')+'>입력</option></select>'+
      '<button class="ghost" style="padding:4px 9px" onclick="gpioWrite('+p.logical+',1)">1</button>'+
      '<button class="ghost" style="padding:4px 9px" onclick="gpioWrite('+p.logical+',0)">0</button>'+
      '<button class="ghost" style="padding:4px 9px" onclick="gpioToggle('+p.logical+')">Toggle</button>'+
      '</div></div>';
  });
  document.getElementById('gpioPins').innerHTML=ph;
  const hex=x=>('0000'+x.toString(16).toUpperCase()).slice(-4);
  let rh='<h2 style="font-size:14px">레지스터 뷰</h2>';
  ['A','B'].forEach(port=>{
    const R=regs[port];
    rh+='<div style="font:12px var(--mono)">GPIO'+port+': MODER=0x'+hex(R.MODER)+' ODR=0x'+hex(R.ODR)+' IDR=0x'+hex(R.IDR)+'</div>';
    let strip='';
    for(let b=15;b>=0;b--){ const s=(R.ODR>>b)&1; strip+='<span style="color:'+(s?'var(--acc2)':'#44517a')+'">'+s+'</span>'; }
    rh+='<div style="font:11px var(--mono);letter-spacing:1px;margin:2px 0 6px">ODR[15:0] '+strip+'</div>';
  });
  document.getElementById('gpioRegs').innerHTML=rh;
  document.getElementById('gpioLog').innerHTML=gpiolog.slice(-8).map(l=>'&gt; '+esc(l)).join('<br>');
}

/* ---- CAN 순수 코어: 프레임 → 액추에이터 명령 (prvConvertCanToActuatorCmd 재현) ---- */
function canParseFrame(id,dlc,data){
  const r={emergency:false,throttle:null,fan:null,action:'무시 (미정의 ID)'};
  if(id===0x123){
    r.emergency=true; r.throttle=0; r.fan=0;
    r.action='🚨 긴급정지 — prvEmergencyStop() 즉시 호출 (스로틀 0, 팬 OFF)';
  }else if(id===0x200){
    if(dlc>=2){ r.throttle=(data[0]&0xFF)|((data[1]&0xFF)<<8); r.action='스로틀 명령 — duty16='+r.throttle+' (data[0]|data[1]<<8)'; }
    else r.action='스로틀 명령 — DLC<2 라 무시';
  }else if(id===0x201){
    if(dlc>=1){ r.fan=data[0]?1:0; r.action='팬 명령 — state='+r.fan; }
    else r.action='팬 명령 — DLC<1 라 무시';
  }
  return r;
}
function canBitTime(dlc,baud){
  const bits=47+8*dlc;               // SOF+ID+RTR+CTRL+DATA+CRC+ACK+EOF+IFS (스터핑 제외)
  const us=bits/baud*1e6;
  const worstUs=(bits*1.2)/baud*1e6; // 최대 20% 비트 스터핑 가정
  return {bits:bits, us:us, worstUs:worstUs};
}

const canSim={bus:[],head:0,tail:0,count:0,ring:new Array(16).fill(null),throttle:0,fan:0};

function canReadHex(id){ const v=document.getElementById(id).value.trim(); const n=parseInt(v,16); return isNaN(n)?0:(n&0xFF); }
function canInitInputs(){
  let h='';
  for(let i=0;i<8;i++) h+='<input id="canD'+i+'" value="00" maxlength="2" style="width:46px;background:#0b1120;color:#d6e4ff;border:1px solid var(--line);border-radius:7px;padding:5px;font:12px var(--mono);text-align:center"> ';
  document.getElementById('canDataIn').innerHTML=h;
  document.getElementById('canDlc').innerHTML=[0,1,2,3,4,5,6,7,8].map(n=>'<option'+(n===0?' selected':'')+'>'+n+'</option>').join('');
  canRender();
}
function canSend(){
  const idStr=document.getElementById('canId').value.trim();
  let id=parseInt(idStr,16); if(isNaN(id)) id=parseInt(idStr,10)||0; id=id&0x7FF;
  const dlc=parseInt(document.getElementById('canDlc').value,10);
  const data=[]; for(let i=0;i<8;i++) data.push(canReadHex('canD'+i));
  const parsed=canParseFrame(id,dlc,data);
  if(canSim.count<16){ canSim.ring[canSim.head]={id:data?null:null,dlc:dlc,data:data.slice()}; canSim.ring[canSim.head].id=id; canSim.head=(canSim.head+1)%16; canSim.count++; }
  if(parsed.emergency){ canSim.throttle=0; canSim.fan=0; }
  else if(id===0x200 && parsed.throttle!==null){ canSim.throttle=parsed.throttle; }
  else if(id===0x201 && parsed.fan!==null){ canSim.fan=parsed.fan; }
  const bt=canBitTime(dlc,500000);
  canSim.bus.unshift({id:data?null:null,dlc:dlc,data:data.slice(),action:parsed.action,bt:bt}); canSim.bus[0].id=id;
  if(canSim.bus.length>12) canSim.bus.pop();
  canRender();
}
function canRender(){
  const tp=(canSim.throttle<=1000)?canSim.throttle/10:100;
  let h='<h2 style="font-size:14px">액추에이터 상태</h2>';
  h+='<div class="dim">스로틀 duty16 = <b>'+canSim.throttle+'</b> → '+tp.toFixed(1)+'%</div>';
  h+='<div class="bar"><i style="width:'+tp+'%"></i></div>';
  h+='<div class="dim">팬: <span style="width:14px;height:14px;border-radius:50%;background:'+(canSim.fan?'#3ddc84':'#3a4a6b')+';display:inline-block"></span> '+(canSim.fan?'ON':'OFF')+'</div>';
  h+='<h2 style="font-size:14px">수신 링버퍼 (ISR 경로)</h2><div class="dim">count='+canSim.count+' · head='+canSim.head+' · tail='+canSim.tail+' (세마포어로 태스크 깨움)</div>';
  h+='<h2 style="font-size:14px">버스 로그 (최신 상단)</h2>';
  canSim.bus.forEach(f=>{
    h+='<div class="finding info" style="margin:4px 0;padding:6px 10px"><div class="t" style="font-size:13px">ID 0x'+f.id.toString(16).toUpperCase().padStart(3,'0')+' · DLC '+f.dlc+' · '+esc(f.action)+'</div>'+
      '<div class="dim" style="font-size:11.5px">데이터: '+f.data.slice(0,f.dlc).map(b=>b.toString(16).padStart(2,'0')).join(' ')+' · 프레임 ≈'+f.bt.bits+'bit ≈'+f.bt.us.toFixed(0)+'µs @500kbps (스터핑 최악 ≈'+f.bt.worstUs.toFixed(0)+'µs)</div></div>';
  });
  document.getElementById('canOut').innerHTML=h;
}
function canPreset(k){
  const D=id=>document.getElementById(id);
  const zero=()=>{ for(let i=0;i<8;i++) D('canD'+i).value='00'; };
  if(k==='emg'){ D('canId').value='0x123'; D('canDlc').value='0'; zero(); }
  if(k==='thr'){ D('canId').value='0x200'; D('canDlc').value='2'; zero(); D('canD0').value='F4'; D('canD1').value='01'; } /* 0x01F4=500 → 50% */
  if(k==='fan'){ D('canId').value='0x201'; D('canDlc').value='1'; zero(); D('canD0').value='01'; }
}

/* ============================================================
   인터럽트 우선순위 계산기 (순수 코어 + DOM)
============================================================ */
function irqCompute(group,preempt,sub,prioBits,libMax){
  const preBits=group;                 // 선점 비트 수 = 그룹 값
  const subBits=prioBits-group;        // 서브 비트 수
  const maxPre=(1<<preBits)-1;
  const maxSub=(1<<subBits)-1;
  const eff=((preempt<<subBits)|sub)&0xF;   // 4bit 유효 우선순위 값
  const reg=eff<<(8-prioBits);              // 8bit NVIC 레지스터 값
  const maxSyscall=libMax<<(8-prioBits);    // configMAX_SYSCALL_INTERRUPT_PRIORITY
  const safe=eff>=libMax;                   // 유효값 ≥ 임계 → FromISR 안전
  return {preBits,subBits,maxPre,maxSub,eff,reg,maxSyscall,safe};
}
function irqCalc(){
  const G=parseInt(document.getElementById('irqGroup').value,10)||0;
  const pre=parseInt(document.getElementById('irqPre').value,10)||0;
  const sub=parseInt(document.getElementById('irqSub').value,10)||0;
  const bits=parseInt(document.getElementById('irqBits').value,10)||4;
  const libMax=parseInt(document.getElementById('irqLibMax').value,10)||5;
  const r=irqCompute(G,pre,sub,bits,libMax);
  const hex=x=>'0x'+x.toString(16).toUpperCase();
  let h='<div class="dim">그룹 '+G+' → 선점 <b>'+r.preBits+'bit</b> + 서브 <b>'+r.subBits+'bit</b></div>';
  h+='<div class="dim">유효 범위: 선점 0~'+r.maxPre+' · 서브 0~'+r.maxSub+'</div>';
  h+='<table style="margin-top:8px"><tr><th>유효 우선순위</th><th>NVIC 레지스터</th><th>FromISR</th></tr>';
  h+='<tr><td>'+r.eff+'</td><td>'+hex(r.reg)+'</td><td class="'+(r.safe?'pass':'fail')+'">'+(r.safe?'✅ 안전':'❌ 위험')+'</td></tr></table>';
  h+='<p class="dim" style="font:12px var(--mono)">레지스터 = 유효값 &lt;&lt; '+(8-bits)+' = '+r.eff+' &lt;&lt; '+(8-bits)+' = '+hex(r.reg)+'</p>';
  document.getElementById('irqOut').innerHTML=h;
  const f='<div class="dim" style="font:12px var(--mono)">configMAX_SYSCALL = '+libMax+' &lt;&lt; '+(8-bits)+' = '+hex(r.maxSyscall)+'</div>'+
    '<div class="dim" style="margin:8px 0">유효값 '+r.eff+' vs 임계 '+libMax+' → '+
    (r.safe?'<b style="color:var(--acc2)">유효값 ≥ 임계 ⇒ FromISR API 호출 가능 ✅</b>':'<b style="color:var(--crit)">유효값 < 임계 ⇒ 우선순위가 너무 높아 FromISR 호출 불가 ❌</b>')+'</div>'+
    '<p class="dim">예) bsp_can.c 의 CAN1_RX0 = 그룹4·선점5 → 유효값 5 = 임계 5 → 안전 (xSemaphoreGiveFromISR 사용 가능)</p>';
  document.getElementById('irqFreertos').innerHTML=f;
}
function irqPreset(g,pre,sub){
  document.getElementById('irqGroup').value=g;
  document.getElementById('irqPre').value=pre;
  document.getElementById('irqSub').value=sub;
  irqCalc();
}
function irqTable(){
  let t='<table><tr><th>그룹</th><th>선점bit</th><th>서브bit</th><th>선점 범위</th><th>서브 범위</th></tr>';
  for(let g=0;g<=4;g++){
    const pb=g, sb=4-g;
    t+='<tr><td>'+g+'</td><td>'+pb+'</td><td>'+sb+'</td><td>0~'+((1<<pb)-1)+'</td><td>0~'+((1<<sb)-1)+'</td></tr>';
  }
  t+='</table><p class="dim">우리 프로젝트는 NVIC_PRIORITYGROUP_4 (선점 4bit, 서브 0bit) 를 사용합니다.</p>';
  document.getElementById('irqTable').innerHTML=t;
}

'''

if 'function gpioComputeRegs' not in html:
    html = html.rstrip()
    html = html.replace('</script>', JS_BLOCK + '\n</script>', 1)

# ===========================================================================
# D. 초기화 호출 주입 — 모든 const 선언 이후(</script> 직전)에 배치 (TDZ 방지)
# ===========================================================================
if '/* 지연 초기화2 */' not in html:
    html = html.replace(
        '</script>',
        '\n/* 지연 초기화2 */\ngpioRender();\ncanInitInputs();\nirqCalc();\nirqTable();\n</script>',
        1
    )

open(HTML, 'w', encoding='utf-8').write(html)
print('시뮬레이터/인터럽트 주입 완료:', len(html), 'bytes')
