#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
build_learning7.py — 사용자 학습효과 개선 3종 (멱등)
  1) 리뷰 찾기 모드: 결함을 도구가 알려주기 전에 사용자가 먼저 찾는 훈련
  2) 자가설명 프롬프트: 계산기/RTA 결과 뒤 "왜?" 서술 + 모범 설명
  3) 오늘의 30분 루프: 홈에 읽기→작성→회상→훈련 순서 제시
build_learning.py + 3 + 4 + 5 + 6 실행 후 실행한다.
"""
HTML = '/home/user/learning_program.html'
html = open(HTML, encoding='utf-8').read()

# ===========================================================================
# A. 리뷰 탭: 찾기 모드 진입 버튼 + 패널
# ===========================================================================
if 'id="findModeWrap"' not in html:
    html = html.replace(
        '  <div id="reviewOut"></div>',
        '''  <div class="card">
    <button class="act" style="background:var(--acc2)" onclick="startFindMode()">🔍 찾기 모드 — 결함을 직접 찾아보기</button>
    <span class="dim"> (도구가 알려주기 전에, 먼저 스스로 찾는 훈련)</span>
  </div>
  <div id="findModeWrap"></div>
  <div id="reviewOut"></div>'''
    )

# ===========================================================================
# B. 홈: 오늘의 30분 루프 카드
# ===========================================================================
if 'id="dailyCard"' not in html:
    html = html.replace(
        '  <div id="progress" class="card"></div>',
        '''  <div id="progress" class="card"></div>
  <div class="card" id="dailyCard">
    <h2>☀️ 오늘의 30분 학습 루프</h2>
    <p class="dim">아래 순서대로 진행하세요. 완료 시 체크하면 진행률이 표시됩니다. (읽기→작성→회상→훈련)</p>
    <div class="chk"><input type="checkbox" onchange="dailyProgress()"><div><b>1️⃣ 읽기 (5분)</b> — 학습 경로에서 오늘의 모듈 읽기 <button class="ghost" style="padding:3px 8px;margin-left:8px" onclick="goTab('path')">이동</button></div></div>
    <div class="chk"><input type="checkbox" onchange="dailyProgress()"><div><b>2️⃣ 작성 (10분)</b> — 코딩 과제 1개 풀기 <button class="ghost" style="padding:3px 8px;margin-left:8px" onclick="goTab('missions')">이동</button></div></div>
    <div class="chk"><input type="checkbox" onchange="dailyProgress()"><div><b>3️⃣ 회상 (5분)</b> — 주관식 퀴즈 <button class="ghost" style="padding:3px 8px;margin-left:8px" onclick="goTab('quiz');setQuizMode('sa')">이동</button></div></div>
    <div class="chk"><input type="checkbox" onchange="dailyProgress()"><div><b>4️⃣ 훈련 (10분)</b> — 리뷰 찾기 모드로 결함 직접 찾기 <button class="ghost" style="padding:3px 8px;margin-left:8px" onclick="goTab('review');startFindMode()">이동</button></div></div>
    <div class="bar"><i id="dailyBar"></i></div>
  </div>'''
    )

# ===========================================================================
# C. JS 주입
# ===========================================================================
JS_BLOCK = r'''

/* ============================================================
   개선 1 — 리뷰 찾기 모드 (도구가 알려주기 전에 스스로 찾기)
============================================================ */
const ALL_RULES = RULES.concat(EMBEDDED_RULES);
const FIND_MODEL_WHY = '가장 치명적인 것은 R3(워치독 피드 단일 실패점)입니다. IWDG 피드가 CAN 태스크에만 있어서, 펌웨어 업데이트가 그 태스크를 정지시키면 플래싱 도중 워치독 타임아웃으로 시스템이 리셋되어 펌웨어가 손상(벽돌)될 수 있습니다. 해결책은 전용 워치독 태스크가 각 태스크의 하트비트를 확인한 뒤에만 피드하도록 바꾸는 것입니다.';

function codeWithLines(code){
  return code.split('\n').map(function(l,i){ return '<span class="ln">'+(i+1)+'</span>'+esc(l); }).join('\n');
}
function computeFindScore(userIds, actualIds){
  const u=new Set(userIds), a=new Set(actualIds);
  const hit=[], miss=[], falsePos=[];
  a.forEach(function(id){ if(u.has(id)) hit.push(id); else miss.push(id); });
  u.forEach(function(id){ if(!a.has(id)) falsePos.push(id); });
  return {hit:hit, miss:miss, falsePos:falsePos};
}
function startFindMode(){
  const wrap=document.getElementById('findModeWrap');
  if(!wrap) return;
  const code=SAMPLES.bad;
  let h='<div class="card"><h2>🔍 찾기 모드 — 결함을 직접 찾아보세요</h2>';
  h+='<p class="dim">아래 코드를 읽고, <b>위반되었다고 생각하는 결함 유형을 체크</b>하고, 가장 심각한 결함과 그 이유를 한 줄로 설명해 보세요. 작성 후 제출하면 채점됩니다.</p>';
  h+='<pre class="code">'+codeWithLines(code)+'</pre>';
  h+='<h2>결함 유형 체크 (위반했다고 생각하는 것)</h2><div id="findChecks">';
  ALL_RULES.forEach(function(r){
    h+='<label style="display:block;margin:4px 0"><input type="checkbox" class="findchk" value="'+r.id+'"> <b>'+r.id+'</b> '+esc(r.t)+'</label>';
  });
  h+='</div>';
  h+='<h2>✍️ 가장 심각한 결함과 이유 (자가설명)</h2>';
  h+='<textarea id="findWhy" style="min-height:70px" placeholder="예: 워치독 피드가 ... 태스크에만 있어서 ... 할 때 리셋되어 벽돌이 됩니다."></textarea>';
  h+='<div><button class="act" onclick="submitFindMode()">제출 & 채점</button></div>';
  h+='<div id="findResult"></div></div>';
  wrap.innerHTML=h;
  wrap.scrollIntoView({block:'nearest'});
}
function submitFindMode(){
  const checked=[].slice.call(document.querySelectorAll('#findChecks .findchk')).filter(function(c){return c.checked;}).map(function(c){return c.value;});
  const actual=ALL_RULES.filter(function(r){ return r.test(SAMPLES.bad); }).map(function(r){return r.id;});
  const sc=computeFindScore(checked, actual);
  let h='<div class="card"><h2>채점 결과</h2>';
  h+='<p class="dim" style="font-size:14px">정확히 찾음 <b style="color:var(--acc2)">'+sc.hit.length+'</b> · 놓침 <b style="color:var(--crit)">'+sc.miss.length+'</b> · 오탐 <b style="color:var(--warn)">'+sc.falsePos.length+'</b></p>';
  if(sc.hit.length) h+='<div class="finding info"><div class="t">✅ 맞춤: '+sc.hit.join(', ')+'</div></div>';
  if(sc.miss.length){
    h+='<div class="finding crit"><div class="t">❌ 놓친 결함: '+sc.miss.join(', ')+'</div><div class="dim">'+
      sc.miss.map(function(id){ const r=ALL_RULES.find(function(x){return x.id===id;}); return id+': '+r.t; }).join('<br>')+'</div></div>';
  }
  if(sc.falsePos.length) h+='<div class="finding warn"><div class="t">⚠️ 오탐 (실제로는 위반 아님): '+sc.falsePos.join(', ')+'</div></div>';
  h+='<div class="finding info"><div class="t">정답 — 코드에 실제로 위반된 규칙</div><div class="dim">'+actual.join(', ')+'</div></div>';
  h+='<div class="finding info"><div class="t">✍️ 모범 자가설명</div><div class="dim">'+esc(FIND_MODEL_WHY)+'</div></div>';
  h+='<p class="dim">내가 쓴 설명과 모범 설명을 비교해 보세요. "놓친 결함"은 규칙의 정의를 다시 읽어보세요.</p>';
  h+='</div>';
  document.getElementById('findResult').innerHTML=h;
}

/* ============================================================
   개선 2 — 자가설명 프롬프트 (계산기·RTA 결과 뒤 "왜?")
   함수 선언 호이스팅을 피하기 위해 런타임 할당으로 래핑
============================================================ */
var __calcTimer=calcTimer, __calcCan=calcCan, __calcIwdg=calcIwdg,
    __calcAdc=calcAdc, __calcUart=calcUart, __calcPwm=calcPwm, __calcRTA=calcRTA;

function seBox(outId, model){
  const out=document.getElementById(outId);
  if(!out || !out.querySelector || !out.appendChild) return;
  try{ const old=out.querySelector('.sebox'); if(old) old.remove(); }catch(e){}
  const div=document.createElement('div'); div.className='sebox';
  div.innerHTML='<div style="margin-top:10px;border-top:1px dashed var(--line);padding-top:8px">'
    +'<div class="dim">🧠 자가설명: 결과가 왜 이렇게 나왔을까? 먼저 스스로 설명해 보고, 아래를 눌러 확인하세요.</div>'
    +'<button class="ghost" style="padding:5px 10px" onclick="this.nextElementSibling.style.display=\'block\'">모범 설명 보기</button>'
    +'<div style="display:none;background:#0e2236;border-radius:7px;padding:8px 12px;font-size:13px;color:var(--acc2)">'+esc(model)+'</div></div>';
  out.appendChild(div);
}
calcTimer=function(){ __calcTimer(); seBox('tmOut','타이머: freq = clk / ((PSC+1)·(ARR+1)). 180MHz ÷ 9000 = 20kHz 이므로 (ARR+1)=9000, ARR=8999 가 정확히 나누어떨어지는 값입니다.'); };
calcCan=function(){ __calcCan(); seBox('canOut','CAN: 총 TQ=1+BS1+BS2=18, PSC=45MHz/(500k×18)=5. 샘플포인트=(1+BS1)/총TQ=14/18≈77.8% 로 권장 범위(75~87%) 안입니다.'); };
calcIwdg=function(){ __calcIwdg(); seBox('wdgOut','IWDG: timeout=분주×(reload+1)/LSI = 64×500/32kHz = 1000ms. 리로드 499 에 +1 이 되어 500 이 되는 점이 핵심입니다.'); };
calcAdc=function(){ __calcAdc(); seBox('adcOut','ADC: 12bit=4096단계, 1LSB=3.3V/4096≈0.806mV. 2048은 절반 → 1.65V 입니다.'); };
calcUart=function(){ __calcUart(); seBox('uOut','UART: USARTDIV=45MHz/(16×115200)=24.41 → 정수부 24 + 분수 7/16. 오차는 분수를 16분의 1 단위로 반올림하며 생기는 약 0.1% 입니다.'); };
calcPwm=function(){ __calcPwm(); seBox('pwOut','PWM: CCR=ARR×duty/100=8999×0.5≈4500. 코드가 0~1000 퍼밀 단위를 쓰므로 50% = 500퍼밀 입니다.'); };
calcRTA=function(){ __calcRTA(); seBox('rtaOut','RTA: 이용률은 C/T 의 합, 응답시간은 고정점 반복입니다. 낮은 우선순위 태스크(Firmware)의 응답시간이 큰 이유는 자신보다 높은 모든 태스크의 실행 시간을 더해 기다려야 하기 때문입니다.'); };

/* ============================================================
   개선 3 — 오늘의 30분 루프
============================================================ */
function goTab(id){
  navBtns.forEach(function(x){ x.classList.toggle('on', x.dataset.s===id); });
  document.querySelectorAll('main>section').forEach(function(s){ s.classList.toggle('on', s.id===id); });
  window.scrollTo({top:0,behavior:'smooth'});
}
function dailyProgress(){
  const card=document.getElementById('dailyCard');
  if(!card) return;
  const boxes=[].slice.call(card.querySelectorAll('input[type=checkbox]'));
  const done=boxes.filter(function(b){return b.checked;}).length;
  const bar=document.getElementById('dailyBar');
  if(bar) bar.style.width=(done/boxes.length*100)+'%';
}

'''

if 'function startFindMode' not in html:
    html = html.rstrip()
    html = html.replace('</script>', JS_BLOCK + '\n</script>', 1)

open(HTML, 'w', encoding='utf-8').write(html)
print('개선 3종 주입 완료:', len(html), 'bytes')
