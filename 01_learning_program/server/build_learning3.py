#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# [보관용 — 실행하지 말 것]
# 이 HTML 을 만들어 온 일회성 UI 주입 스크립트다. 결과는 이미
# learning_program.html 에 반영되어 있고, 작성 당시의 절대 경로를
# 그대로 들고 있어 지금 실행하면 실패한다. 자세한 내용은 server/README.md 참조.
# 소스 스냅샷 갱신은 refresh_html_sources.py 를 쓴다.
"""
build_learning3.py — 개선안 1/2/3 주입 (멱등)
  1) 학습 경로 탭: M1→M6 큐레이션 + 파일·줄 점프
  2) 주관식 퀴즈: 회상형 10문항
  3) 코딩 과제 탭: 자체 완결형 빈칸 채우기 5개 미션 + 검증
build_learning.py 실행 후 이 스크립트를 실행한다.
"""
HTML = '/home/user/learning_program.html'
html = open(HTML, encoding='utf-8').read()

# ===========================================================================
# A. 내비게이션 버튼 추가 (멱등)
# ===========================================================================
if 'data-s="path"' not in html:
    html = html.replace(
        '    <button data-s="build">🔨 빌드</button>',
        '    <button data-s="build">🔨 빌드</button>\n    <button data-s="path">🗺️ 학습 경로</button>\n    <button data-s="missions">🎯 코딩 과제</button>'
    )

# ===========================================================================
# B. 섹션 HTML 주입 (멱등)
# ===========================================================================
if 'section id="path"' not in html:
    path_section = '''<!-- ============ PATH ============ -->
<section id="path">
  <h1>🗺️ 학습 경로 (M1 → M6)</h1>
  <p class="dim">파일 나열이 아니라 <b>읽는 순서</b>를 제공합니다. 각 모듈의 <b>파일·줄 점프</b>를 눌러 핵심 코드로 바로 이동하세요. 완료 시 체크하면 홈 진도표에 반영됩니다.</p>
  <div id="pathWrap"></div>
</section>

<!-- ============ MISSIONS ============ -->
<section id="missions">
  <h1>🎯 코딩 과제 (빈칸 채우기)</h1>
  <p class="dim">이 프로젝트의 <b>수정 사항(안전 결함 수정)</b>을 직접 한 줄씩 작성해 봅니다. <b>검증</b>은 오프라인으로, <b>gcc 확인</b>은 서버로 컴파일합니다. "정답 보기"는 꼭 시도한 뒤에 누르세요.</p>
  <div id="missionWrap"></div>
</section>

'''
    html = html.replace('<div class="foot">', path_section + '<div class="foot">')

# ===========================================================================
# C. 퀴즈 탭: 모드 토글 + 주관식 영역 (멱등)
# ===========================================================================
if 'id="quizModeBar"' not in html:
    html = html.replace(
        '''    <div id="quizWrap"></div>
    <button class="act" onclick="gradeQuiz()">✅ 채점하기</button>
    <div id="quizScore"></div>''',
        '''    <div id="quizModeBar" style="margin:10px 0">
      <button class="act" id="mcBtn" onclick="setQuizMode('mc')">객관식 18문항</button>
      <button class="ghost" id="saBtn" onclick="setQuizMode('sa')">주관식 10문항</button>
    </div>
    <div id="quizWrap"></div>
    <div id="quizSubWrap" style="display:none"></div>
    <button class="act" onclick="gradeAny()">✅ 채점하기</button>
    <div id="quizScore"></div>
    <div id="quizSubScore"></div>'''
    )

# 홈 소개 카드에 항목 추가 (멱등)
html = html.replace(
    '<li><b>빌드</b> — 문법 검사 + gcc·cppcheck·MISRA 통합 검사</li>',
    '<li><b>빌드</b> — 문법 검사 + gcc·cppcheck·MISRA 통합 검사</li>\n        <li><b>학습 경로</b> — M1→M6 순서로 읽을 파일·핵심 라인 큐레이션</li>\n        <li><b>코딩 과제</b> — 빈칸을 채워 안전 코드를 직접 작성하고 검증</li>'
)

# ===========================================================================
# D. JS 데이터 + 함수 주입 (멱등)
# ===========================================================================
JS_BLOCK = r'''

/* ============================================================
   학습 경로 (개선안 1)
============================================================ */
const PATH=[
 {id:'M1',icon:'🔍',t:'코드 리뷰와 결함 식별',
  goal:'결함은 "동작할 때"가 아니라 "고장날 때" 드러난다 — 실행 전에 찾는 사고 훈련',
  concept:['워치독 피드의 주체가 하나뿐이면, 그 태스크가 멈추는 순간 시스템 전체가 무방비가 된다(단일 실패점).','안전 명령(긴급정지)은 일반 명령과 같은 경로를 타면 안 된다 — 큐 가득 참/뮤텍스 실패로 유실되면 치명적.'],
  reads:[],
  activity:'리뷰 탭 → "예시 ① 결함 있는 원본" 로드 → 결함 2건을 직접 찾고, 검사 실행으로 확인하세요.'},
 {id:'M2',icon:'🛡️',t:'안전 재설계 (워치독 + 긴급정지)',
  goal:'결함을 "설계"로 제거하기 — 수정판 main_fixed.c 의 핵심',
  concept:['전용 워치독 태스크: 모든 태스크의 하트비트를 확인한 뒤에만 피드 → 단일 실패점 제거.','긴급 정지는 큐를 거치지 않고 즉시 안전 상태(PWM 0) 적용, 뮤텍스는 portMAX_DELAY 로 획득 보장.'],
  reads:[
   {file:'main_fixed.c',line:163,fn:'vTask_Watchdog',why:'수정 1 핵심 — 하트비트 확인 후에만 IWDG 피드'},
   {file:'main_fixed.c',line:367,fn:'prvEmergencyStop',why:'수정 2 핵심 — 즉시 안전 상태 + 획득 보장 뮤텍스'},
   {file:'main_fixed.c',line:201,fn:'vTask_CAN_Rx',why:'긴급 ID 수신 시 prvEmergencyStop() 즉시 호출'}
  ],
  activity:'코딩 과제 탭 → 미션 1·2 로 직접 한 줄씩 작성해 보세요.'},
 {id:'M3',icon:'🔌',t:'BSP / HAL (하드웨어 추상화)',
  goal:'MCU 페리퍼럴을 논리 계층으로 감싸는 법 — AUTOSAR MCAL 의 축소판',
  concept:['ISR 은 가벼워야 한다: 링버퍼에 저장 + 세마포어로 태스크 깨우기만.','플래싱(부트로더)은 스케줄러 정지 중이므로 워치독을 직접 피드해야 벽돌을 막는다.'],
  reads:[
   {file:'bsp/bsp_can.c',line:109,fn:'CAN RX ISR',why:'인터럽트+링버퍼+세마포어 구조'},
   {file:'bsp/bsp_iwdg.c',line:9,fn:'IWDG_Init',why:'LSI 기반 타임아웃 계산'},
   {file:'bsp/bootloader.c',line:43,fn:'Bootloader_FlashNewFirmware',why:'삭제 전·1KB마다 IWDG 피드'},
   {file:'bsp/bsp_pwm.c',line:10,fn:'PWM_Init',why:'TIM1 20kHz 생성'}
  ],
  activity:'코드 학습 탭에서 bsp/*.c 를 훑고, 빌드 탭에서 프로젝트 전체 체크를 돌려보세요.'},
 {id:'M4',icon:'📐',t:'실시간 이론 (RTA)',
  goal:'"우선순위가 옳다"를 감이 아니라 수학으로 증명',
  concept:['Liu–Layland 상한 n(2^(1/n)−1) 은 n→∞ 이면 ln2≈69.3% (충분조건).','응답시간 분석: R=C+B+Σ⌈R/T_j⌉·C_j 고정점 반복, R≤D 이면 스케줄가능.'],
  reads:[
   {file:'docs/docs_realtime_theory_proof.md',line:1,fn:'증명 문서',why:'정리 4개 증명 스케치 + 태스크셋 판정'},
   {file:'rta_analysis.py',line:1,fn:'계산 스크립트',why:'실제 계산 코드 (파이썬)'}
  ],
  activity:'RTA 계산기 탭에서 Debug 의 C 를 6→0.1 로 바꿔보고 이용률 변화를 확인하세요.'},
 {id:'M5',icon:'📏',t:'실측 방법론',
  goal:'"돌아간다"가 아니라 "측정된다"로 승격',
  concept:['스택: uxTaskGetStackHighWaterMark() — 최악 시나리오에서 측정해야 의미가 있다.','안전 판정은 평균이 아니라 최악값(WCET) 기준이다.'],
  reads:[
   {file:'docs/docs_measurement_plan.md',line:1,fn:'실측 계획서',why:'5개 측정 항목의 방법론과 합격 기준'},
   {file:'docs/docs_measurement_report_template.md',line:1,fn:'보고서 템플릿',why:'측정 결과 기록 양식'}
  ],
  activity:'실측 5항목(스택·부하·지터·E2E·워치독)을 종이에 그려보고, 하드웨어가 생기면 그대로 수행하세요.'},
 {id:'M6',icon:'🏭',t:'AUTOSAR / ISO 26262',
  goal:'이 프로젝트를 업계 표준 언어로 번역',
  concept:['ISO 26262: ASIL(QM<A<B<C<D), HARA(S·E·C), Safety Goal→Mechanism→Safe State.','AUTOSAR: SWC 는 RTE 로만 통신 — 우리의 "태스크 간 큐로만 통신"과 같은 원칙.'],
  reads:[
   {file:'docs/docs_autosar_iso26262.md',line:1,fn:'표준 요약',why:'표준 개념과 프로젝트 대응표'},
   {file:'docs/docs_motivation_letter.md',line:1,fn:'동기서 초안',why:'이 경험을 독일식 서사로 정리한 예'}
  ],
  activity:'대응표(워치독=Program Flow Monitoring, 긴급정지=Safe State)를 외우고 동기서 한 문단으로 정리하세요.'}
];

function renderPath(){
  const wrap=document.getElementById('pathWrap');
  wrap.innerHTML='';
  PATH.forEach(m=>{
    const on=!!state.progress[m.id];
    const card=document.createElement('div'); card.className='card';
    let h='<div style="display:flex;align-items:center;gap:10px"><span style="font-size:22px">'+m.icon+'</span><div><b style="font-size:16px">'+m.id+' · '+m.t+'</b><br><span class="dim">'+m.goal+'</span></div></div>';
    h+='<h2>핵심 개념</h2><ul style="margin:4px 0 0 18px">';
    m.concept.forEach(c=>{ h+='<li>'+c+'</li>'; });
    h+='</ul>';
    if(m.reads.length){
      h+='<h2>읽기 — 파일·줄 점프</h2>';
      m.reads.forEach(r=>{
        h+='<div class="finding info" style="cursor:pointer" onclick="jumpToCode(\''+r.file+'\','+r.line+')"><div class="t">📄 '+r.file+' · '+r.line+'행 · '+r.fn+'</div><div class="dim">'+r.why+' — 클릭하면 코드 탭으로 이동합니다 →</div></div>';
      });
    }
    h+='<h2>실습</h2><p class="dim">'+m.activity+'</p>';
    h+='<div class="chk"><input type="checkbox" '+(on?'checked':'')+' data-m="'+m.id+'"><div><b>이 모듈 완료</b> <span class="dim">(홈 진도표에 반영)</span></div></div>';
    card.innerHTML=h;
    card.querySelector('input').addEventListener('change',e=>{ state.progress[m.id]=e.target.checked; saveState(); renderProgress(); });
    wrap.appendChild(card);
  });
}

function jumpToCode(file,line){
  const sel=document.getElementById('codeSel');
  const idx=ALL_FILES.findIndex(f=>f.p===file);
  if(idx>=0){ sel.value=idx; showCode(line); }
  navBtns.forEach(x=>x.classList.remove('on'));
  document.querySelectorAll('main>section').forEach(s=>s.classList.remove('on'));
  const t=document.querySelector('[data-s="code"]'); if(t)t.classList.add('on');
  document.getElementById('code').classList.add('on');
  window.scrollTo({top:0,behavior:'smooth'});
}

/* showCode 재정의 — 줄 하이라이트 지원 (함수 호이스팅으로 전역 적용) */
function showCode(hl){
  const f=ALL_FILES[document.getElementById('codeSel').value];
  const lines=f.c.split('\n');
  const view=document.getElementById('codeView');
  view.innerHTML=lines.map(function(l,i){
    const n=i+1;
    const code=renderLine(l,i);
    if(hl && n===hl){ return '<span data-hl="'+n+'" style="background:#3a2c14">'+code+'</span>'; }
    return code;
  }).join('\n');
  const note=NOTES[f.p]||'이 파일의 한 줄씩 주석을 읽어보세요.';
  document.getElementById('codeNote').textContent='📄 '+f.p+' — '+lines.length+'줄 · 💡 '+note;
  if(hl){ const el=view.querySelector('[data-hl="'+hl+'"]'); if(el){ setTimeout(function(){ el.scrollIntoView({block:'center'}); },30); } }
}

/* ============================================================
   주관식 퀴즈 (개선안 2 — 회상형)
============================================================ */
const SUBQUIZ=[
 {m:'M1',q:'원본 코드에서 워치독(IWDG) 피드를 담당했던 태스크는? (수정 전)',a:['can수신태스크','can수신','canrx','can_rx','can'],hint:'IWDG_ReloadCounter() 가 vTask_CAN_Rx 안에만 있었습니다.'},
 {m:'M2',q:'긴급 정지 시 스로틀 PWM 듀티를 몇(0~1000)으로 설정해야 하나요?',a:['0'],hint:'prvEmergencyStop() 참고 — 출력 차단.'},
 {m:'M2',q:'prvEmergencyStop() 이 뮤텍스를 기다릴 때 사용한 FreeRTOS 상수는?',a:['portmax_delay','portmaxdelay'],hint:'획득을 "보장"하는 무한 대기 값.'},
 {m:'M4',q:'Liu–Layland 상한은 n→∞ 일 때 어떤 값으로 수렴하나요?',a:['ln2','ln 2','0.693','0.6931','69.3','69.3%','0.693147'],hint:'자연로그 2 (ln 2).'},
 {m:'M4',q:'응답시간 분석 식의 ⌈R/T_j⌉ 항의 의미는?',a:['고우선순위작업수','고우선순위태스크수','고우선순위작업수','간섭횟수','간섭 횟수','작업수','작업 수'],hint:'구간 [0,R) 에 릴리스된 고우선순위 태스크의 실행 횟수.'},
 {m:'M5',q:'태스크 스택의 남은 용량을 런타임에 측정하는 FreeRTOS API 이름은?',a:['uxtaskgetstackhighwatermark'],hint:'"High Water Mark" 가 들어갑니다.'},
 {m:'M3',q:'NUCLEO-F446RE 에서 CAN1 RX 핀은 어디인가요?',a:['pb8'],hint:'CAN1_TX 는 PB9 입니다.'},
 {m:'M3',q:'printf 가 USART2 로 출력되도록 재정의하는 newlib 함수 이름은?',a:['_write','write'],hint:'밑줄(_)로 시작하는 표준 출력 함수입니다.'},
 {m:'M6',q:'ISO 26262 에서 가장 엄격한 ASIL 등급은?',a:['asild','asil d','d'],hint:'QM < A < B < C < ?'},
 {m:'M6',q:'전용 워치독 태스크의 하트비트 감시에 대응하는 ISO 26262 안전 메커니즘은?',a:['programflowmonitoring','프로그램흐름감시','프로그램 흐름 감시','흐름감시'],hint:'영문 약자 P.F.M — 프로그램 흐름 감시.'}
];

function norm(s){ return (s||'').toLowerCase().replace(/[\s_()\[\].]/g,''); }
function setQuizMode(m){
  state.quizMode=m;
  const mc=document.getElementById('quizWrap'), sa=document.getElementById('quizSubWrap');
  const mcb=document.getElementById('mcBtn'), sab=document.getElementById('saBtn');
  if(m==='sa'){
    mc.style.display='none'; sa.style.display='block';
    mcb.classList.remove('act'); mcb.classList.add('ghost');
    sab.classList.remove('ghost'); sab.classList.add('act');
    document.getElementById('quizScore').style.display='none';
    renderSubQuiz();
  }else{
    mc.style.display='block'; sa.style.display='none';
    sab.classList.remove('act'); sab.classList.add('ghost');
    mcb.classList.remove('ghost'); mcb.classList.add('act');
    document.getElementById('quizScore').style.display='block';
  }
}
function renderSubQuiz(){
  const wrap=document.getElementById('quizSubWrap');
  wrap.innerHTML='';
  SUBQUIZ.forEach((q,qi)=>{
    const d=document.createElement('div'); d.className='q'; d.id='sq'+qi;
    d.innerHTML='<b>'+q.m+'</b> '+q.q+'<br>'+
      '<input type="text" id="sa'+qi+'" placeholder="답을 입력하세요" style="width:70%;background:#0b1120;color:#d6e4ff;border:1px solid var(--line);border-radius:7px;padding:7px 10px;margin:8px 0;font:14px var(--mono)">'+
      '<div class="dim" style="font-size:12px">힌트: '+q.hint+'</div>'+
      '<div class="exp" id="sexp'+qi+'"></div>';
    wrap.appendChild(d);
  });
  document.getElementById('quizSubScore').innerHTML='';
}
function gradeAny(){ if(state.quizMode==='sa'){ gradeSubQuiz(); } else { gradeQuiz(); } }
function gradeSubQuiz(){
  let score=0;
  SUBQUIZ.forEach((q,qi)=>{
    const val=document.getElementById('sa'+qi).value;
    const box=document.getElementById('sq'+qi);
    const exp=document.getElementById('sexp'+qi);
    const nv=norm(val);
    const ok=q.a.some(x=>norm(x)===nv);
    exp.classList.add('show');
    if(ok){ score++; box.classList.add('right'); box.classList.remove('wrong'); exp.innerHTML='✅ 정답!'; }
    else { box.classList.add('wrong'); box.classList.remove('right'); exp.innerHTML='❌ 정답: '+q.a[0]+' — <span class="dim">'+q.hint+'</span>'; }
  });
  document.getElementById('quizSubScore').innerHTML='<div class="card"><h2>주관식 결과: '+score+' / '+SUBQUIZ.length+'</h2>'+
    '<div class="bar"><i style="width:'+(score/SUBQUIZ.length*100)+'%"></i></div>'+
    '<p class="dim">주관식은 "알고 있는지"를 정직하게 확인합니다. 틀린 항목은 해당 모듈을 다시 읽어보세요.</p></div>';
}

/* ============================================================
   코딩 과제 (개선안 3 — 빈칸 채우기 + 검증)
   템플릿은 자체 완결형: 서버가 스텁 헤더를 자동 포함하므로
   정답 제출 시 gcc 컴파일이 통과한다.
============================================================ */
const MISSIONS=[
 {id:1,t:'워치독 피드 한 줄 채우기',lv:'쉬움',learn:'단일 실패점 제거 — 전용 태스크에서 조건부 피드',
  marker:'/* ==== 여기에 코드를 작성하세요 ==== */',
  must:'IWDG_ReloadCounter',
  hint:'IWDG 피드 함수 이름은 IWDG_ReloadCounter() 입니다.',
  template:'// 미션 1: 모든 태스크의 하트비트가 살아있을 때 IWDG 를 피드하세요.\n#include "task.h"\nvoid IWDG_ReloadCounter(void);   // BSP 함수 (실제로는 bsp_iwdg.h 에 선언)\nvolatile uint32_t ulHeartbeat_CAN_Rx, prevC, ulHeartbeat_Sensor, prevS;\nTickType_t xLastWakeTime;\nvoid vTask_Watchdog(void *pvParameters)\n{\n    for (;;)\n    {\n        if (ulHeartbeat_CAN_Rx != prevC && ulHeartbeat_Sensor != prevS)\n        {\n            /* ==== 여기에 코드를 작성하세요 ==== */\n        }\n        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));\n    }\n}\n',
  solution:'// 미션 1: 모든 태스크의 하트비트가 살아있을 때 IWDG 를 피드하세요.\n#include "task.h"\nvoid IWDG_ReloadCounter(void);   // BSP 함수 (실제로는 bsp_iwdg.h 에 선언)\nvolatile uint32_t ulHeartbeat_CAN_Rx, prevC, ulHeartbeat_Sensor, prevS;\nTickType_t xLastWakeTime;\nvoid vTask_Watchdog(void *pvParameters)\n{\n    for (;;)\n    {\n        if (ulHeartbeat_CAN_Rx != prevC && ulHeartbeat_Sensor != prevS)\n        {\n            IWDG_ReloadCounter();\n        }\n        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));\n    }\n}\n'},
 {id:2,t:'긴급 정지 즉시 적용',lv:'쉬움',learn:'안전 명령은 큐를 거치지 않고 즉시 처리',
  marker:'/* ==== 여기에 코드를 작성하세요 ==== */',
  must:'prvEmergencyStop()',
  hint:'즉시 안전 상태를 적용하는 함수는 prvEmergencyStop() 입니다.',
  template:'// 미션 2: 긴급 정지 CAN ID 수신 시 즉시 안전 상태를 적용하세요.\n#include "task.h"\n#include "bsp_can.h"              // CAN_Message_t, CAN_Receive\n#define CAN_ID_EMERGENCY 0x123U\nvoid prvEmergencyStop(void);      // 즉시 안전 상태 적용 함수\nvoid vTask_CAN_Rx(void *pvParameters)\n{\n    CAN_Message_t xRxMsg;\n    for (;;)\n    {\n        if (CAN_Receive(&xRxMsg, pdMS_TO_TICKS(10)) == pdPASS)\n        {\n            if (xRxMsg.ID == CAN_ID_EMERGENCY)\n            {\n                /* ==== 여기에 코드를 작성하세요 ==== */\n            }\n        }\n    }\n}\n',
  solution:'// 미션 2: 긴급 정지 CAN ID 수신 시 즉시 안전 상태를 적용하세요.\n#include "task.h"\n#include "bsp_can.h"              // CAN_Message_t, CAN_Receive\n#define CAN_ID_EMERGENCY 0x123U\nvoid prvEmergencyStop(void);      // 즉시 안전 상태 적용 함수\nvoid vTask_CAN_Rx(void *pvParameters)\n{\n    CAN_Message_t xRxMsg;\n    for (;;)\n    {\n        if (CAN_Receive(&xRxMsg, pdMS_TO_TICKS(10)) == pdPASS)\n        {\n            if (xRxMsg.ID == CAN_ID_EMERGENCY)\n            {\n                prvEmergencyStop();\n            }\n        }\n    }\n}\n'},
 {id:3,t:'큐 전송 실패 오류 카운터',lv:'중간',learn:'에러를 "조용히 무시"하지 않고 기록(진단)',
  marker:'/* ==== 여기에 코드를 작성하세요 ==== */',
  must:'ulErrCount_ActuatorQueueFull++',
  hint:'명령 유실 횟수를 기록하는 카운터: ulErrCount_ActuatorQueueFull++',
  template:'// 미션 3: 큐 전송 실패 시 명령 유실을 카운터로 기록하세요.\n#include "queue.h"\nvolatile uint32_t ulErrCount_ActuatorQueueFull = 0;\nvoid *xQueue_ActuatorCmd;   // 실제로는 전역 큐 핸들\nint xCmd;\nvoid send_cmd(void)\n{\n    if (xQueueSend(xQueue_ActuatorCmd, &xCmd, 0) != pdPASS)\n    {\n        /* ==== 여기에 코드를 작성하세요 ==== */\n    }\n}\n',
  solution:'// 미션 3: 큐 전송 실패 시 명령 유실을 카운터로 기록하세요.\n#include "queue.h"\nvolatile uint32_t ulErrCount_ActuatorQueueFull = 0;\nvoid *xQueue_ActuatorCmd;   // 실제로는 전역 큐 핸들\nint xCmd;\nvoid send_cmd(void)\n{\n    if (xQueueSend(xQueue_ActuatorCmd, &xCmd, 0) != pdPASS)\n    {\n        ulErrCount_ActuatorQueueFull++;\n    }\n}\n'},
 {id:4,t:'주기 지연 API 교정',lv:'중간',learn:'주기 드리프트 누적 방지 — vTaskDelayUntil',
  marker:'        vTaskDelay(xPeriod); /* ==== 교체하세요 ==== */',
  must:'vTaskDelayUntil',
  hint:'정밀 주기를 위해 vTaskDelayUntil(&xLastWakeTime, xPeriod) 를 사용하세요.',
  template:'// 미션 4: 주기가 밀리지 않도록 지연 API 를 교정하세요.\n#include "task.h"\nvoid vTask_Sensor(void *pvParameters)\n{\n    const TickType_t xPeriod = pdMS_TO_TICKS(10U);\n    TickType_t xLastWakeTime = xTaskGetTickCount();\n    for (;;)\n    {\n        /* 센서 읽기 */\n        vTaskDelay(xPeriod); /* ==== 교체하세요 ==== */\n    }\n}\n',
  solution:'// 미션 4: 주기가 밀리지 않도록 지연 API 를 교정하세요.\n#include "task.h"\nvoid vTask_Sensor(void *pvParameters)\n{\n    const TickType_t xPeriod = pdMS_TO_TICKS(10U);\n    TickType_t xLastWakeTime = xTaskGetTickCount();\n    for (;;)\n    {\n        /* 센서 읽기 */\n        vTaskDelayUntil(&xLastWakeTime, xPeriod);\n    }\n}\n'},
 {id:5,t:'뮤텍스 반환 채우기',lv:'어려움',learn:'뮤텍스 미반환 = 데드락 — 자원 반환의 중요성',
  marker:'/* ==== 여기에 코드를 작성하세요 ==== */',
  must:'xSemaphoreGive',
  hint:'사용이 끝난 뮤텍스는 xSemaphoreGive(xSemaphore_Actuator) 로 반환해야 합니다.',
  template:'// 미션 5: 뮤텍스 사용 후 반환을 채우세요 (누락 시 데드락).\n#include "semphr.h"\ntypedef struct { uint32_t commandId; uint16_t duty; uint8_t state; uint32_t sourceCanId; } ActuatorCmd_t;\nvoid *xSemaphore_Actuator;   // 실제로는 뮤텍스 핸들\nvoid prvApplyActuatorCommand(ActuatorCmd_t *c);\nvoid vTask_Actuator(void *pvParameters)\n{\n    ActuatorCmd_t xCmd;\n    if (xSemaphoreTake(xSemaphore_Actuator, portMAX_DELAY) == pdTRUE)\n    {\n        prvApplyActuatorCommand(&xCmd);\n        /* ==== 여기에 코드를 작성하세요 ==== */\n    }\n}\n',
  solution:'// 미션 5: 뮤텍스 사용 후 반환을 채우세요 (누락 시 데드락).\n#include "semphr.h"\ntypedef struct { uint32_t commandId; uint16_t duty; uint8_t state; uint32_t sourceCanId; } ActuatorCmd_t;\nvoid *xSemaphore_Actuator;   // 실제로는 뮤텍스 핸들\nvoid prvApplyActuatorCommand(ActuatorCmd_t *c);\nvoid vTask_Actuator(void *pvParameters)\n{\n    ActuatorCmd_t xCmd;\n    if (xSemaphoreTake(xSemaphore_Actuator, portMAX_DELAY) == pdTRUE)\n    {\n        prvApplyActuatorCommand(&xCmd);\n        xSemaphoreGive(xSemaphore_Actuator);\n    }\n}\n'},
];

function renderMissions(){
  const wrap=document.getElementById('missionWrap');
  wrap.innerHTML='';
  MISSIONS.forEach(m=>{
    const card=document.createElement('div'); card.className='card';
    card.innerHTML='<h2>미션 '+m.id+'. '+m.t+' <span class="pill '+(m.lv==='쉬움'?'ok':m.lv==='중간'?'info':'warn')+'">'+m.lv+'</span></h2>'+
      '<p class="dim">배우는 것: '+m.learn+'</p>'+
      '<textarea id="mis'+m.id+'" class="misCode" style="min-height:200px">'+esc(m.template)+'</textarea>'+
      '<div><button class="act" onclick="checkMission('+m.id+')">✅ 검증 (오프라인)</button>'+
      '<button class="ghost" onclick="missionGcc('+m.id+')">🔨 gcc 컴파일 확인</button>'+
      '<button class="ghost" onclick="revealMission('+m.id+')">👀 정답 보기</button>'+
      '<button class="ghost" onclick="resetMission('+m.id+')">↺ 초기화</button></div>'+
      '<div class="dim" style="margin-top:6px">힌트: '+m.hint+'</div>'+
      '<div id="misOut'+m.id+'"></div>';
    wrap.appendChild(card);
  });
}
function checkMission(id){
  const m=MISSIONS[id-1];
  const code=document.getElementById('mis'+id).value;
  const out=document.getElementById('misOut'+id);
  const errs=[];
  if(code.includes(m.marker)) errs.push('아직 빈칸(marker)이 남아 있습니다.');
  if(!code.includes(m.must)) errs.push('필수 코드("'+m.must+'")가 없습니다.');
  const syn=clientSyntaxCheck(code);
  errs.push.apply(errs, syn.map(s=>s.line+'행 '+s.msg));
  if(errs.length===0){
    out.innerHTML='<div class="finding info"><div class="t">✅ 오프라인 검증 통과</div><div class="dim">빈칸이 채워졌고 필수 코드가 포함되었으며 문법 구조도 정상입니다. gcc 확인으로 컴파일까지 검증하세요.</div></div>';
  }else{
    out.innerHTML='<div class="finding crit"><div class="t">❌ 미완료</div><div class="dim">'+errs.map(esc).join('<br>')+'</div></div>';
  }
}
function revealMission(id){
  const m=MISSIONS[id-1];
  document.getElementById('mis'+id).value=m.solution;
  document.getElementById('misOut'+id).innerHTML='<div class="finding warn"><div class="t">정답을 공개했습니다</div><div class="dim">어디가 달랐는지 비교해 보고, 초기화 후 다시 시도해 보세요.</div></div>';
}
function resetMission(id){
  const m=MISSIONS[id-1];
  document.getElementById('mis'+id).value=m.template;
  document.getElementById('misOut'+id).innerHTML='';
}
async function missionGcc(id){
  const code=document.getElementById('mis'+id).value;
  const out=document.getElementById('misOut'+id);
  out.innerHTML='<div class="dim">gcc -fsyntax-only 검사 중...</div>';
  try{
    const r=await fetch('/api/build',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({code})});
    const j=await r.json();
    if(j.ok && !j.output){ out.innerHTML='<div class="finding info"><div class="t">✅ 컴파일 통과 (경고·오류 0)</div></div>'; }
    else { out.innerHTML='<div class="finding crit"><div class="t">❌ 컴파일 오류</div><pre class="code">'+esc(j.output)+'</pre></div>'; }
  }catch(e){
    out.innerHTML='<div class="finding warn"><div class="t">서버 연결 실패</div><div class="dim">gcc 컴파일 확인은 라이브 프리뷰에서 실행하세요.</div></div>';
  }
}

'''

if 'const PATH=' not in html:
    html = html.rstrip()
    html = html.replace('</script>', JS_BLOCK + '\n</script>', 1)

# ===========================================================================
# E. 초기화 호출 주입 — 모든 const 선언 이후(</script> 직전)에 배치 (TDZ 방지)
# ===========================================================================
if '/* 지연 초기화 */' not in html:
    html = html.replace(
        '</script>',
        '\n/* 지연 초기화 */\nrenderPath();\nrenderMissions();\n</script>',
        1
    )

open(HTML, 'w', encoding='utf-8').write(html)
print('개선안 1/2/3 주입 완료:', len(html), 'bytes')
