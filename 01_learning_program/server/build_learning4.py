#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
build_learning4.py — 임베디드 전용 특화 주입 (멱등)
  1) 임베디드 전용 검사 규칙 7종 (ISR·volatile·FromISR·워치독 튜닝 등)
  2) 임베디드 계산기 탭 (타이머/CAN/IWDG/ADC/UART/PWM — BSP 코드가 하는 계산의 체험)
build_learning.py + build_learning3.py 실행 후 실행한다.
"""
HTML = '/home/user/learning_program.html'
html = open(HTML, encoding='utf-8').read()

# ===========================================================================
# A. 내비게이션: 계산기 탭 추가
# ===========================================================================
if 'data-s="calc"' not in html:
    html = html.replace(
        '    <button data-s="missions">🎯 코딩 과제</button>',
        '    <button data-s="missions">🎯 코딩 과제</button>\n    <button data-s="calc">🧮 임베디드 계산기</button>'
    )

# ===========================================================================
# B. 계산기 섹션 주입
# ===========================================================================
if 'section id="calc"' not in html:
    calc_section = '''<!-- ============ CALC ============ -->
<section id="calc">
  <h1>🧮 임베디드 계산기</h1>
  <p class="dim">BSP 코드가 내부에서 수행하는 계산을 직접 체험합니다. <b>①</b> 타이머 ARR(bsp_pwm.c) · <b>②</b> CAN 비트타이밍(bsp_can.c) · <b>③</b> IWDG 타임아웃(bsp_iwdg.c) · <b>④</b> ADC 변환(bsp_adc.c) · <b>⑤</b> UART BRR(bsp_uart.c) · <b>⑥</b> PWM 듀티 변환.</p>

  <div class="grid">
    <div class="card">
      <h2>① 타이머/PWM — ARR·프리스케일러</h2>
      <p class="dim">freq = clk / ((PSC+1)·(ARR+1)) — bsp_pwm.c 의 PWM_Init 과 동일</p>
      <label class="dim">타이머 클록 (MHz)</label>
      <input id="tmClk" type="number" value="180" style="width:100%;background:#0b1120;color:#d6e4ff;border:1px solid var(--line);border-radius:7px;padding:7px;margin:4px 0">
      <label class="dim">목표 주파수 (Hz)</label>
      <input id="tmFreq" type="number" value="20000" style="width:100%;background:#0b1120;color:#d6e4ff;border:1px solid var(--line);border-radius:7px;padding:7px;margin:4px 0">
      <button class="act" onclick="calcTimer()">계산</button>
      <div id="tmOut" class="dim" style="margin-top:8px"></div>
    </div>

    <div class="card">
      <h2>② CAN 비트타이밍 — 프리스케일러·샘플포인트</h2>
      <p class="dim">prescaler = PCLK1 / (baud·(1+BS1+BS2)) — bsp_can.c 의 prvCanSetTiming</p>
      <label class="dim">PCLK1 (MHz)</label>
      <input id="canClk" type="number" value="45" style="width:100%;background:#0b1120;color:#d6e4ff;border:1px solid var(--line);border-radius:7px;padding:7px;margin:4px 0">
      <label class="dim">보레이트 (bps)</label>
      <input id="canBaud" type="number" value="500000" style="width:100%;background:#0b1120;color:#d6e4ff;border:1px solid var(--line);border-radius:7px;padding:7px;margin:4px 0">
      <label class="dim">BS1 (TQ) / BS2 (TQ)</label>
      <div style="display:flex;gap:8px">
        <input id="canBs1" type="number" value="13" style="width:50%;background:#0b1120;color:#d6e4ff;border:1px solid var(--line);border-radius:7px;padding:7px">
        <input id="canBs2" type="number" value="4" style="width:50%;background:#0b1120;color:#d6e4ff;border:1px solid var(--line);border-radius:7px;padding:7px">
      </div>
      <button class="act" onclick="calcCan()" style="margin-top:8px">계산</button>
      <div id="canOut" class="dim" style="margin-top:8px"></div>
    </div>

    <div class="card">
      <h2>③ IWDG 타임아웃 — 분주·리로드</h2>
      <p class="dim">timeout(ms) = prescaler·(reload+1) / LSI(kHz) — bsp_iwdg.c 의 IWDG_Init</p>
      <label class="dim">LSI (kHz)</label>
      <input id="wdgLsi" type="number" value="32" style="width:100%;background:#0b1120;color:#d6e4ff;border:1px solid var(--line);border-radius:7px;padding:7px;margin:4px 0">
      <label class="dim">분주</label>
      <select id="wdgPsc" style="width:100%;margin:4px 0">
        <option value="4">4</option><option value="8">8</option><option value="16">16</option>
        <option value="32">32</option><option value="64" selected>64</option>
        <option value="128">128</option><option value="256">256</option>
      </select>
      <label class="dim">리로드 (0~4095)</label>
      <input id="wdgRld" type="number" value="499" style="width:100%;background:#0b1120;color:#d6e4ff;border:1px solid var(--line);border-radius:7px;padding:7px;margin:4px 0">
      <button class="act" onclick="calcIwdg()">계산</button>
      <div id="wdgOut" class="dim" style="margin-top:8px"></div>
    </div>

    <div class="card">
      <h2>④ ADC 변환 — 카운트 ↔ 전압</h2>
      <p class="dim">12bit: V = counts·Vref/4096 — bsp_adc.c 의 ADC_ReadChannel</p>
      <label class="dim">기준 전압 Vref (V)</label>
      <input id="adcVref" type="number" value="3.3" style="width:100%;background:#0b1120;color:#d6e4ff;border:1px solid var(--line);border-radius:7px;padding:7px;margin:4px 0">
      <label class="dim">입력 (방식 선택)</label>
      <select id="adcMode" style="width:100%;margin:4px 0">
        <option value="c2v">카운트(0~4095) → 전압</option>
        <option value="v2c">전압(V) → 카운트</option>
      </select>
      <input id="adcVal" type="number" value="2048" style="width:100%;background:#0b1120;color:#d6e4ff;border:1px solid var(--line);border-radius:7px;padding:7px;margin:4px 0">
      <button class="act" onclick="calcAdc()">계산</button>
      <div id="adcOut" class="dim" style="margin-top:8px"></div>
    </div>

    <div class="card">
      <h2>⑤ UART BRR — 보레이트 오차</h2>
      <p class="dim">USARTDIV = APB / (16·baud) — bsp_uart.c 의 UART_Init</p>
      <label class="dim">APB 클록 (MHz)</label>
      <input id="uClk" type="number" value="45" style="width:100%;background:#0b1120;color:#d6e4ff;border:1px solid var(--line);border-radius:7px;padding:7px;margin:4px 0">
      <label class="dim">보레이트 (bps)</label>
      <input id="uBaud" type="number" value="115200" style="width:100%;background:#0b1120;color:#d6e4ff;border:1px solid var(--line);border-radius:7px;padding:7px;margin:4px 0">
      <button class="act" onclick="calcUart()">계산</button>
      <div id="uOut" class="dim" style="margin-top:8px"></div>
    </div>

    <div class="card">
      <h2>⑥ PWM 듀티 변환 — % ↔ 퍼밀 ↔ CCR</h2>
      <p class="dim">CCR = ARR·duty/100 — bsp_pwm.c 의 PWM_SetDuty (단위: 0~1000 퍼밀)</p>
      <label class="dim">듀티 (%)</label>
      <input id="pwDuty" type="number" value="50" style="width:100%;background:#0b1120;color:#d6e4ff;border:1px solid var(--line);border-radius:7px;padding:7px;margin:4px 0">
      <label class="dim">ARR</label>
      <input id="pwArr" type="number" value="8999" style="width:100%;background:#0b1120;color:#d6e4ff;border:1px solid var(--line);border-radius:7px;padding:7px;margin:4px 0">
      <button class="act" onclick="calcPwm()">계산</button>
      <div id="pwOut" class="dim" style="margin-top:8px"></div>
    </div>
  </div>
</section>

'''
    html = html.replace('<div class="foot">', calc_section + '<div class="foot">')

# ===========================================================================
# C. 리뷰 탭: 임베디드 전용 검사 버튼 추가
# ===========================================================================
if 'runEmbeddedReview' not in html:
    html = html.replace(
        '<div><button class="act" onclick="runReview()">🔍 검사 실행</button>',
        '<div><button class="act" onclick="runReview()">🔍 검사 실행</button>\n  <button class="act" style="background:var(--warn)" onclick="runEmbeddedReview()">🔬 임베디드 전용 검사</button>'
    )

# 홈 소개 카드에 항목 추가
html = html.replace(
    '<li><b>코딩 과제</b> — 빈칸을 채워 안전 코드를 직접 작성하고 검증</li>',
    '<li><b>코딩 과제</b> — 빈칸을 채워 안전 코드를 직접 작성하고 검증</li>\n        <li><b>임베디드 계산기</b> — 타이머·CAN·IWDG·ADC·UART·PWM 계산 체험</li>'
)

# ===========================================================================
# D. JS 주입 (임베디드 규칙 + 계산기)
# ===========================================================================
JS_BLOCK = r'''

/* ============================================================
   임베디드 전용 검사 규칙 (ISR·volatile·FromISR·워치독 튜닝)
   ※ 휴리스틱 — 정식 검증은 MISRA/정적분석 도구 사용
============================================================ */
const EMBEDDED_RULES=[
 {id:'E1',sev:'crit',t:'ISR/콜백 내 블로킹·지연 호출',
  test:c=>/(HAL_Delay|vTaskDelay|printf)\s*\(/.test(c) && /(IRQHandler|Callback|ISR|_IRQ)/.test(c),
  m:'인터럽트 핸들러/콜백 안에서 블로킹 지연이나 printf 를 호출하면 ISR 이 길어져 상위 우선순위 인터럽트를 지연시킵니다.',
  f:'ISR 은 "저장+통지"만 하고 처리는 태스크로 위임하세요 (xSemaphoreGiveFromISR + 태스크 수신).'},
 {id:'E2',sev:'crit',t:'ISR 내 일반(비 FromISR) FreeRTOS API',
  test:c=>/(xQueueSend|xSemaphoreTake|xSemaphoreGive|vTaskDelayUntil|vTaskDelay)\s*\(/.test(c) && /(IRQHandler|Callback|ISR)/.test(c) && !/FromISR/.test(c),
  m:'ISR 컨텍스트에서 일반 API(예: xQueueSend)를 호출하면 스케줄러 문맥에서 벗어나 오동작할 수 있습니다.',
  f:'FromISR 버전 + pxHigherPriorityTaskWoken + portYIELD_FROM_ISR 패턴을 사용하세요.'},
 {id:'E3',sev:'warn',t:'RTOS 태스크 내 HAL_Delay 블로킹',
  test:c=>/HAL_Delay\s*\(\s*(?:[2-9]\d|\d{3,})\s*\)/.test(c),
  m:'긴 HAL_Delay(≥2ms)는 CPU 를 점유해 다른 태스크·워치독 피드를 지연시킵니다.',
  f:'vTaskDelay(틱) 로 대체하거나, 초기화 전용이면 주석으로 명시하세요.'},
 {id:'E4',sev:'warn',t:'ISR 공유 변수 volatile 누락 가능성',
  test:c=>/static\s+(?:volatile\s+)?(uint32_t|uint8_t|BaseType_t|TickType_t)\s+\w+\s*=\s*0/.test(c) && /(Callback|IRQ|ISR|FromISR)/.test(c) && !/volatile/.test(c),
  m:'ISR 과 태스크가 공유하는 변수는 volatile 로 선언하고, 필요 시 임계구역/원자적 접근으로 보호해야 합니다.',
  f:'공유 변수를 volatile 로 선언하세요. 32bit 미만은 임계구역으로 보호.'},
 {id:'E5',sev:'info',t:'메모리 매핑 레지스터 volatile',
  test:c=>/\(\s*\([A-Za-z_]+_TypeDef\s*\*\s*\)\s*0x[0-9A-Fa-f]+/.test(c) && !/volatile/.test(c),
  m:'MMIO 레지스터 포인터는 최적화로 접근이 생략되지 않도록 volatile 이어야 합니다.',
  f:'(volatile ..._TypeDef *) 또는 구조체 멤버를 volatile 로 정의하세요.'},
 {id:'E6',sev:'info',t:'CAN 페이로드 시프트 조립',
  test:c=>/<<\s*8/.test(c) && /data\[/.test(c),
  m:'CAN 페이로드는 리틀엔디언이라 시프트 조립합니다. 오프셋·부호 확장 실수가 잦은 지점입니다.',
  f:'명시적 캐스팅((uint16_t)data[1]<<8 | data[0])과 DLC 경계 검사로 방어하세요.'},
 {id:'E7',sev:'warn',t:'워치독 타임아웃 튜닝 확인',
  test:c=>/IWDG_Init\s*\(\s*(\d{1,3})\s*\)/.test(c),
  m:'워치독 타임아웃이 감시 주기(예: 100ms)보다 크게 여유 있어야 오탐 리셋이 없습니다. (권장 5~10배)',
  f:'IWDG_Init(1000) 처럼 검사 주기의 충분한 배수로 설정하고, 고장 주입 실험으로 검증하세요.'}
];

function runEmbeddedReview(){
  const c=document.getElementById('codeIn').value;
  const out=document.getElementById('reviewOut');
  if(c.trim().length<10){ out.innerHTML='<div class="card dim">검사할 코드를 입력하세요.</div>'; return; }
  const hits=EMBEDDED_RULES.filter(r=>r.test(c));
  const sevRank={crit:0,warn:1,info:2};
  hits.sort((a,b)=>sevRank[a.sev]-sevRank[b.sev]);
  let h='<div class="card"><h2>🔬 임베디드 전용 검사 결과</h2><p class="dim">ISR·volatile·FromISR·워치독 등 임베디드 특화 규칙 7종 검사. (휴리스틱 — 정식 검증은 gcc/cppcheck/MISRA 사용)</p>';
  if(hits.length===0){
    h+='<div class="finding info"><div class="t">✅ 임베디드 특화 결함 없음</div></div>';
  }else{
    hits.forEach(r=>{
      const badge=r.sev==='crit'?'<span class="pill crit">치명</span>':r.sev==='warn'?'<span class="pill warn">경고</span>':'<span class="pill info">정보</span>';
      h+='<div class="finding '+r.sev+'"><div class="t">'+r.t+' '+badge+' <span class="dim">('+r.id+')</span></div><div class="dim">'+r.m+'</div><div class="dim" style="color:var(--acc2)">수정: '+r.f+'</div></div>';
    });
  }
  h+='</div>';
  out.innerHTML=h;
}

/* ============================================================
   임베디드 계산기
============================================================ */
function num(id){ return parseFloat(document.getElementById(id).value); }
function fnum(x,d){ return (isFinite(x)?x:0).toFixed(d||3); }

function calcTimer(){
  const clk=num('tmClk')*1e6, freq=num('tmFreq');
  const out=document.getElementById('tmOut');
  if(!freq||freq<=0){ out.textContent='목표 주파수를 입력하세요'; return; }
  let psc=0, arr=clk/freq-1;
  if(arr>65535){ psc=Math.ceil(clk/(freq*65536))-1; if(psc<0)psc=0; arr=clk/(freq*(psc+1))-1; }
  const actual=clk/((psc+1)*(arr+1));
  const err=(actual-freq)/freq*100;
  let h='<b>PSC</b> = '+Math.round(psc)+' · <b>ARR</b> = '+Math.round(arr)+'<br>';
  h+='실주파수 = '+fnum(actual,2)+' Hz (오차 '+fnum(err,2)+'%)<br>';
  if(arr>65535) h+='<span style="color:var(--crit)">ARR 이 16bit(65535) 초과 — 프리스케일러 필요</span><br>';
  if(Math.abs(err)>2) h+='<span style="color:var(--warn)">오차 &gt;2% — 더 큰 ARR·PSC 조합 검토</span>';
  out.innerHTML=h;
}
function calcCan(){
  const clk=num('canClk')*1e6, baud=num('canBaud'), bs1=num('canBs1'), bs2=num('canBs2');
  const out=document.getElementById('canOut');
  const tq=1+bs1+bs2;
  const pscRaw=clk/(baud*tq);
  const psc=Math.round(pscRaw);
  const actual=clk/(psc*tq);
  const sp=(1+bs1)/tq*100;
  const err=(actual-baud)/baud*100;
  let h='총 TQ = '+tq+' · <b>프리스케일러</b> ≈ '+fnum(pscRaw,2)+' (반올림 '+psc+')<br>';
  h+='샘플포인트 = '+fnum(sp,1)+'% (권장 75~87%)<br>';
  h+='실보레이트 = '+fnum(actual,0)+' bps (오차 '+fnum(err,2)+'%)<br>';
  if(Math.abs(pscRaw-psc)>0.01) h+='<span style="color:var(--warn)">프리스케일러가 정수가 아님 — BS1/BS2 조정으로 정수화 권장</span><br>';
  if(sp<70||sp>90) h+='<span style="color:var(--warn)">샘플포인트가 권장 범위(75~87%)를 벗어남</span>';
  out.innerHTML=h;
}
function calcIwdg(){
  const lsi=num('wdgLsi'), psc=parseFloat(document.getElementById('wdgPsc').value), rld=num('wdgRld');
  const out=document.getElementById('wdgOut');
  const t=psc*(rld+1)/lsi;
  let h='타임아웃 = <b>'+fnum(t,2)+' ms</b> (분주 '+psc+', 리로드 '+rld+')<br>';
  if(t<100) h+='<span style="color:var(--crit)">100ms 미만 — 너무 짧아 오탐 리셋 위험</span><br>';
  else if(t>5000) h+='<span style="color:var(--warn)">5초 초과 — 고장 검출이 너무 느림</span><br>';
  else h+='<span style="color:var(--acc2)">✅ 감시 주기(100ms) 대비 적절한 범위</span><br>';
  h+='<span class="dim">※ LSI 는 17~47kHz 로 편차가 커 실측 보정이 필요합니다.</span>';
  out.innerHTML=h;
}
function calcAdc(){
  const vref=num('adcVref'), mode=document.getElementById('adcMode').value, val=num('adcVal');
  const out=document.getElementById('adcOut');
  const lsb=vref/4096;
  let h='1 LSB = '+fnum(lsb,4)+' mV<br>';
  if(mode==='c2v'){ h+='전압 = <b>'+fnum(val*lsb,3)+' V</b> ('+Math.round(val)+' counts)'; }
  else { h+='카운트 = <b>'+Math.round(val/lsb)+'</b> ('+val+' V → '+fnum(val/lsb,1)+')'; }
  out.innerHTML=h;
}
function calcUart(){
  const clk=num('uClk')*1e6, baud=num('uBaud');
  const out=document.getElementById('uOut');
  const div=clk/(16*baud);
  const mant=Math.floor(div);
  const frac=Math.round((div-mant)*16);
  const actual=clk/(16*(mant+frac/16));
  const err=(actual-baud)/baud*100;
  let h='USARTDIV = '+fnum(div,4)+'<br>';
  h+='BRR = Mantissa '+mant+' + Fraction '+frac+'/16<br>';
  h+='실보레이트 = '+fnum(actual,0)+' bps (오차 '+fnum(err,3)+'%)<br>';
  if(Math.abs(err)>2) h+='<span style="color:var(--warn)">오차 &gt;2% — 수신 신뢰성에 영향</span>';
  else h+='<span style="color:var(--acc2)">✅ 오차 2% 이내</span>';
  out.innerHTML=h;
}
function calcPwm(){
  const duty=num('pwDuty'), arr=num('pwArr');
  const out=document.getElementById('pwOut');
  const ccr=Math.round(arr*duty/100);
  const permille=Math.round(duty*10);
  out.innerHTML='CCR = <b>'+ccr+'</b> · 듀티(퍼밀) = <b>'+permille+'</b>/1000<br><span class="dim">bsp_pwm.c 의 PWM_SetDuty 는 0~1000 퍼밀 단위를 사용합니다.</span>';
}

'''

if 'const EMBEDDED_RULES=' not in html:
    html = html.rstrip()
    html = html.replace('</script>', JS_BLOCK + '\n</script>', 1)

open(HTML, 'w', encoding='utf-8').write(html)
print('임베디드 특화 주입 완료:', len(html), 'bytes')
