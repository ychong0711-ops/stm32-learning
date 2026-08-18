#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# [보관용 — 실행하지 말 것]
# 이 HTML 을 만들어 온 일회성 UI 주입 스크립트다. 결과는 이미
# learning_program.html 에 반영되어 있고, 작성 당시의 절대 경로를
# 그대로 들고 있어 지금 실행하면 실패한다. 자세한 내용은 server/README.md 참조.
# 소스 스냅샷 갱신은 refresh_html_sources.py 를 쓴다.
"""
build_learning6.py — Phase 1: 번들 로더 + manifest 포맷 (멱등)
  - "📦 프로젝트" 탭 추가: 다른 STM32F4·FreeRTOS·C 프로젝트를 로드
  - loadBundle(): ALL_FILES/NOTES/PATH/QUIZ/SUBQUIZ/MISSIONS/MODULES/rtaTasks 를
    런타임에 교체 (const 배열/객체의 내용을 in-place 변형)
  - 샘플 번들(온도 경보 시스템) + 원복 기능 + manifest 스키마 표시
  - sample_bundle.json 파일도 생성
build_learning.py + 3 + 4 + 5 실행 후 실행한다.
"""
import json

HTML = '/home/user/learning_program.html'
html = open(HTML, encoding='utf-8').read()

# ===========================================================================
# 샘플 번들: 다른 프로젝트 (온도 경보 시스템)
# ===========================================================================
def find_line(content, needle):
    for i, l in enumerate(content.split('\n')):
        if needle in l:
            return i + 1
    return 1

TEMP_ALARM_C = """// temp_alarm.c — 온도 경보 태스크 (STM32F4 + FreeRTOS, 교육용 예제)
#include "task.h" // FreeRTOS 태스크 API 를 사용하기 위해 포함합니다.
#include "bsp_uart.h" // UART 출력 함수 선언을 포함합니다.
#include "bsp_gpio.h" // GPIO 제어 함수 선언을 포함합니다.
#define TEMP_THRESHOLD_X100 7500 // 경보 임계값입니다. (75.00℃, 0.01℃ 단위)
#define LED_PIN 5U // 경보 LED 의 논리 핀 번호입니다.
void UART_Send(const char *s); // UART 문자열 전송 함수 선언입니다.
void GPIO_Write(uint32_t pin, uint8_t state); // GPIO 출력 함수 선언입니다.
void vTask_TempAlarm(void *pvParameters) // 온도 경보 태스크를 정의합니다.
{ // 태스크 본문을 시작합니다.
    const TickType_t xPeriod = pdMS_TO_TICKS(100U); // 100ms 주기를 틱으로 변환합니다.
    TickType_t xLastWakeTime = xTaskGetTickCount(); // 정밀 주기용 기준 시각입니다.
    for (;;) // 무한 루프를 시작합니다.
    { // 루프 본문을 시작합니다.
        int32_t lTempX100 = BMP280_ReadTempX100(); // 온도를 0.01℃ 단위로 읽습니다.
        if (lTempX100 > TEMP_THRESHOLD_X100) // 임계값을 초과했는지 확인합니다.
        { // 경보 발생 블록을 시작합니다.
            GPIO_Write(LED_PIN, 1U); // 경보 LED 를 점등합니다.
            UART_Send("ALARM"); // 경보 메시지를 출력합니다.
        } // 경보 발생 블록을 종료합니다.
        else // 임계값 이하인 경우입니다.
        { // 정상 상태 블록을 시작합니다.
            GPIO_Write(LED_PIN, 0U); // 경보 LED 를 소등합니다.
        } // 정상 상태 블록을 종료합니다.
        vTaskDelayUntil(&xLastWakeTime, xPeriod); // 100ms 정밀 주기를 유지합니다.
    } // 무한 루프를 종료합니다.
} // 태스크 함수를 종료합니다.
"""

CLI_C = """// cli.c — 시리얼 명령 파서 (교육용 예제)
#include <stdint.h> // 고정 폭 정수 타입을 사용하기 위해 포함합니다.
void UART_Send(const char *s); // UART 출력 함수 선언입니다.
void cli_handle(uint8_t cmd) // 명령 처리 함수를 정의합니다.
{ // 함수 본문을 시작합니다.
    switch (cmd) // 수신한 명령 값으로 분기합니다.
    { // switch 블록을 시작합니다.
        case 0x01: // LED 켜기 명령인 경우입니다.
            UART_Send("LED ON"); // 응답을 출력합니다.
            break; // 케이스를 종료합니다.
        case 0x02: // LED 끄기 명령인 경우입니다.
            UART_Send("LED OFF"); // 응답을 출력합니다.
            break; // 케이스를 종료합니다.
        default: // 정의되지 않은 명령인 경우입니다.
            UART_Send("UNKNOWN"); // 오류 응답을 출력합니다.
            break; // 기본 케이스를 종료합니다.
    } // switch 블록을 종료합니다.
} // 함수를 종료합니다.
"""

SAMPLE = {
    "title": "온도 경보 시스템 (STM32F4 + FreeRTOS 교육용)",
    "files": [
        {"path": "src/temp_alarm.c", "content": TEMP_ALARM_C},
        {"path": "src/cli.c", "content": CLI_C},
    ],
    "notes": {
        "src/temp_alarm.c": "온도 임계값 판단 + LED/UART 경보 (vTaskDelayUntil 정밀 주기)",
        "src/cli.c": "시리얼 명령 파서 — switch/default 구조",
    },
    "path": [
        {"id": "A1", "icon": "🧭", "t": "구조 파악",
         "goal": "태스크 하나가 '읽기→판단→출력→지연' 순환임을 확인",
         "concept": ["태스크는 무한 루프 + vTaskDelayUntil 로 주기를 만든다.", "임계값 판단은 비교 연산으로, 출력은 GPIO/UART 로 나간다."],
         "reads": [{"file": "src/temp_alarm.c", "line": find_line(TEMP_ALARM_C, "TEMP_THRESHOLD_X100 7500"), "fn": "임계값 정의", "why": "임계값을 0.01℃ 단위 정수로 두는 이유"},
                   {"file": "src/temp_alarm.c", "line": find_line(TEMP_ALARM_C, "if (lTempX100 > TEMP_THRESHOLD_X100)"), "fn": "판단 분기", "why": "초과 시에만 경보"}],
         "activity": "코딩 과제 미션 A1 로 임계값을 직접 채워 보세요."},
        {"id": "A2", "icon": "🔀", "t": "명령 분기 (switch)",
         "goal": "default 절로 미정의 명령을 처리하는 습관",
         "concept": ["switch 에 default 가 없으면 미정의 입력을 조용히 무시한다.", "break 누락은 다음 case 로 떨어지는(fall-through) 버그를 만든다."],
         "reads": [{"file": "src/cli.c", "line": find_line(CLI_C, "default:"), "fn": "default 절", "why": "미정의 명령에 UNKNOWN 응답"}],
         "activity": "주관식 퀴즈에서 '미정의 명령 응답 문자열'을 회상해 보세요."},
        {"id": "A3", "icon": "✅", "t": "검증과 확장",
         "goal": "코드 → 컴파일 → 규칙 검사로 이어지는 습관",
         "concept": ["작성한 코드는 gcc -fsyntax-only 로 문법을, 임베디드 규칙으로 결함 패턴을 검사한다.", "경보 지연(주기)이 요구사양(응답시간)을 만족하는지 RTA 로 확인한다."],
         "reads": [{"file": "src/temp_alarm.c", "line": find_line(TEMP_ALARM_C, "vTaskDelayUntil"), "fn": "정밀 주기", "why": "주기 드리프트 방지"}],
         "activity": "빌드 탭에서 통합 검사를 돌려 보세요."},
    ],
    "quiz": [
        {"m": "A1", "q": "임계값 TEMP_THRESHOLD_X100 이 7500 일 때, 섭씨 몇 도가 경보 기준인가요?", "o": ["75.00℃", "7.5℃", "750℃", "0.75℃"], "a": 0, "w": "0.01℃ 단위이므로 7500 × 0.01 = 75.00℃ 입니다. float 대신 정수를 쓰는 이유는 코드에서 확인하세요."},
        {"m": "A2", "q": "cli.c 의 switch 에서 미정의 명령이 들어오면 출력되는 문자열은?", "o": ["UNKNOWN", "ERROR", "NONE", "무응답"], "a": 0, "w": "default 절이 UNKNOWN 을 출력합니다. default 가 없었다면 조용히 무시됐을 것입니다."},
        {"m": "A3", "q": "vTaskDelay 대신 vTaskDelayUntil 을 쓰는 이유는?", "o": ["주기 드리프트 누적 방지", "코드가 짧아서", "메모리 절약", "우선순위 상승"], "a": 0, "w": "vTaskDelay 는 실행 시간만큼 주기가 밀려 누적되고, vTaskDelayUntil 은 절대 시각 기준으로 주기를 유지합니다."},
    ],
    "subquiz": [
        {"m": "A1", "q": "임계값 초과 시 켜지는 GPIO 핀의 심볼은? (LED_PIN 등)", "a": ["led_pin", "ledpin", "led"], "hint": "temp_alarm.c 의 #define LED_PIN 을 보세요."},
        {"m": "A1", "q": "경보 발생 시 UART 로 출력되는 문자열은?", "a": ["alarm"], "hint": "UART_Send(...) 의 인자를 보세요."},
        {"m": "A2", "q": "미정의 명령에 대한 응답 문자열은?", "a": ["unknown"], "hint": "default 절입니다."},
    ],
    "missions": [
        {"id": 1, "t": "경보 임계값 채우기", "lv": "쉬움", "learn": "임계값을 0.01℃ 단위 정수로 정의",
         "marker": "/* ==== 여기에 코드를 작성하세요 ==== */",
         "must": "7500",
         "hint": "75.00℃ = 7500 (0.01℃ 단위)",
         "template": "// 미션 A1: 경보 임계값(0.01℃ 단위)을 75.00℃ 로 채우세요.\n#include \"task.h\"\nint32_t BMP280_ReadTempX100(void);\nvoid UART_Send(const char *s);\n#define TEMP_THRESHOLD_X100  /* ==== 여기에 코드를 작성하세요 ==== */\nvoid vTask_TempAlarm(void *pvParameters)\n{\n    const TickType_t xPeriod = pdMS_TO_TICKS(100U);\n    TickType_t xLastWakeTime = xTaskGetTickCount();\n    for (;;)\n    {\n        int32_t lTempX100 = BMP280_ReadTempX100();\n        if (lTempX100 > TEMP_THRESHOLD_X100)\n        {\n            UART_Send(\"ALARM\");\n        }\n        vTaskDelayUntil(&xLastWakeTime, xPeriod);\n    }\n}\n",
         "solution": "// 미션 A1: 경보 임계값(0.01℃ 단위)을 75.00℃ 로 채우세요.\n#include \"task.h\"\nint32_t BMP280_ReadTempX100(void);\nvoid UART_Send(const char *s);\n#define TEMP_THRESHOLD_X100  7500\nvoid vTask_TempAlarm(void *pvParameters)\n{\n    const TickType_t xPeriod = pdMS_TO_TICKS(100U);\n    TickType_t xLastWakeTime = xTaskGetTickCount();\n    for (;;)\n    {\n        int32_t lTempX100 = BMP280_ReadTempX100();\n        if (lTempX100 > TEMP_THRESHOLD_X100)\n        {\n            UART_Send(\"ALARM\");\n        }\n        vTaskDelayUntil(&xLastWakeTime, xPeriod);\n    }\n}\n"},
        {"id": 2, "t": "주기 지연 API 교정", "lv": "중간", "learn": "vTaskDelayUntil 로 정밀 주기 유지",
         "marker": "        vTaskDelay(xPeriod); /* ==== 교체하세요 ==== */",
         "must": "vTaskDelayUntil",
         "hint": "vTaskDelayUntil(&xLastWakeTime, xPeriod) 로 교체하세요.",
         "template": "// 미션 A2: 주기 태스크의 지연 API 를 교정하세요.\n#include \"task.h\"\nvoid vTask_Blink(void *pvParameters)\n{\n    const TickType_t xPeriod = pdMS_TO_TICKS(200U);\n    TickType_t xLastWakeTime = xTaskGetTickCount();\n    for (;;)\n    {\n        vTaskDelay(xPeriod); /* ==== 교체하세요 ==== */\n    }\n}\n",
         "solution": "// 미션 A2: 주기 태스크의 지연 API 를 교정하세요.\n#include \"task.h\"\nvoid vTask_Blink(void *pvParameters)\n{\n    const TickType_t xPeriod = pdMS_TO_TICKS(200U);\n    TickType_t xLastWakeTime = xTaskGetTickCount();\n    for (;;)\n    {\n        vTaskDelayUntil(&xLastWakeTime, xPeriod);\n    }\n}\n"},
    ],
    "rta": [
        {"name": "Sensor", "T": 10, "C": 0.8, "p": 3, "D": 10},
        {"name": "Alarm", "T": 100, "C": 0.3, "p": 2, "D": 100},
        {"name": "Console", "T": 200, "C": 1.0, "p": 1, "D": 200},
    ],
}

sample_json = json.dumps(SAMPLE, ensure_ascii=False)

# ===========================================================================
# A. 내비게이션 버튼 추가
# ===========================================================================
if 'data-s="bundle"' not in html:
    html = html.replace(
        '    <button data-s="irq">🎚️ 인터럽트</button>',
        '    <button data-s="irq">🎚️ 인터럽트</button>\n    <button data-s="bundle">📦 프로젝트</button>'
    )

# ===========================================================================
# B. 섹션 HTML 주입
# ===========================================================================
if 'section id="bundle"' not in html:
    section = '''<!-- ============ BUNDLE ============ -->
<section id="bundle">
  <h1>📦 프로젝트 로드 (임베디드 전용 범용화)</h1>
  <p class="dim">manifest.json 번들 포맷으로 <b>다른 STM32F4·FreeRTOS·C 프로젝트</b>를 이 프로그램에 로드합니다. 파일·학습 경로·퀴즈·미션·RTA 태스크가 모두 교체됩니다.</p>
  <div class="grid">
    <div class="card">
      <h2>① 샘플 번들 (1클릭)</h2>
      <p class="dim">다른 프로젝트(온도 경보 시스템)가 로드되는 것을 확인하세요.</p>
      <button class="act" onclick="loadSampleBundle()">📦 샘플 프로젝트 로드</button>
      <button class="ghost" onclick="restoreOriginal()">↺ 원래 교재 복원</button>
    </div>
    <div class="card">
      <h2>② 소스 파일 직접 로드</h2>
      <p class="dim">내 .c/.h 파일 여러 개를 선택하면 코드 학습 탭의 파일 목록이 교체됩니다. (학습 콘텐츠는 유지)</p>
      <input type="file" multiple onchange="addSourceFiles(event)" style="color:var(--txt)">
    </div>
    <div class="card">
      <h2>③ manifest.json 로드</h2>
      <p class="dim">전체 번들(파일+경로+퀴즈+미션+RTA)을 한 번에. 파일 업로드 또는 직접 붙여넣기.</p>
      <input type="file" accept=".json" onchange="loadManifestFile(event)" style="color:var(--txt);margin-bottom:6px">
      <textarea id="manifestIn" style="min-height:130px;font:11.5px var(--mono)" placeholder='{"title":"...","files":[{"path":"x.c","content":"..."}],"path":[...],"quiz":[...],"missions":[...],"rta":[...]}'></textarea>
      <button class="act" onclick="loadManifestText()">로드</button>
    </div>
  </div>
  <div id="bundleStatus" class="card dim"></div>
  <div class="card">
    <h2>manifest.json 스키마 (요약)</h2>
    <pre class="code">{
  "title": "프로젝트명",
  "files":  [ { "path": "src/x.c", "content": "...(소스 전문)..." } ],
  "notes":  { "src/x.c": "파일 설명" },
  "path":   [ { "id":"M1","t":"제목","goal":"...","concept":["..."],
               "reads":[{"file":"src/x.c","line":5,"fn":"f","why":"..."}], "activity":"..." } ],
  "quiz":    [ { "m":"M1","q":"문제","o":["선택지..."],"a":0,"w":"해설" } ],
  "subquiz": [ { "m":"M1","q":"주관식","a":["정답","동의어"],"hint":"힌트" } ],
  "missions":[ { "id":1,"t":"제목","lv":"쉬움","learn":"...","marker":"/* ... */",
                "must":"필수코드","hint":"힌트","template":"...(빈칸 포함)...","solution":"..." } ],
  "rta":     [ { "name":"T","T":10,"C":1,"p":3,"D":10 } ]
}</pre>
    <p class="dim">워크스페이스의 <b>sample_bundle.json</b> 과 <b>MANIFEST_GUIDE.md</b> 를 참고하세요.</p>
  </div>
</section>

'''
    html = html.replace('<div class="foot">', section + '<div class="foot">')

# 홈 소개 카드에 항목 추가
html = html.replace(
    '<li><b>인터럽트 계산기</b> — NVIC 우선순위 + FreeRTOS FromISR 안전성 판정</li>',
    '<li><b>인터럽트 계산기</b> — NVIC 우선순위 + FreeRTOS FromISR 안전성 판정</li>\n        <li><b>프로젝트 로드</b> — manifest.json 으로 다른 임베디드 프로젝트 교체</li>'
)

# ===========================================================================
# C. JS 주입
# ===========================================================================
JS_BLOCK = r'''

/* ============================================================
   Phase 1 — 번들 로더 (다른 임베디드 프로젝트 로드)
   const 배열/객체의 "내용"을 in-place 변형하여 런타임 교체
============================================================ */
const ORIGINAL={};
function captureOriginal(){
  ORIGINAL.files=ALL_FILES.map(f=>({p:f.p,c:f.c}));
  ORIGINAL.notes=Object.assign({},NOTES);
  ORIGINAL.path=PATH.map(m=>JSON.parse(JSON.stringify(m)));
  ORIGINAL.quiz=QUIZ.map(q=>JSON.parse(JSON.stringify(q)));
  ORIGINAL.subquiz=SUBQUIZ.map(q=>JSON.parse(JSON.stringify(q)));
  ORIGINAL.missions=MISSIONS.map(m=>JSON.parse(JSON.stringify(m)));
  ORIGINAL.modules=MODULES.map(m=>JSON.parse(JSON.stringify(m)));
  ORIGINAL.rta=rtaTasks.map(t=>({...t}));
}
function mutate(arr,items){ arr.length=0; (items||[]).forEach(x=>arr.push(x)); }
function clearObj(o){ for(const k in o) delete o[k]; }
function loadBundle(b){
  b=b||{};
  mutate(ALL_FILES,(b.files||[]).map(f=>({p:(f.path||f.p||''),c:(f.content||f.c||'')})));
  clearObj(NOTES); Object.assign(NOTES,b.notes||{});
  mutate(PATH,b.path||[]);
  mutate(QUIZ,b.quiz||[]);
  mutate(SUBQUIZ,b.subquiz||[]);
  mutate(MISSIONS,b.missions||[]);
  mutate(MODULES,(b.path||[]).map(m=>({id:m.id,t:m.t,f:m.goal||''})));
  rtaTasks=(b.rta||[]).map(t=>({...t}));
  state.progress={}; saveState();
  rebuildQuizMods();
  renderCodeSel(); renderPath(); renderQuiz(); renderSubQuiz(); renderMissions();
  renderRTATable(); calcRTA(); renderProgress();
  const s=document.getElementById('bundleStatus');
  if(s) s.innerHTML='✅ 로드됨: '+esc(b.title||'프로젝트')+' — 파일 '+(b.files||[]).length+'개 · 경로 '+(b.path||[]).length+'모듈 · 퀴즈 '+(b.quiz||[]).length+'문항 · 주관식 '+(b.subquiz||[]).length+'문항 · 미션 '+(b.missions||[]).length+'개 · RTA '+(b.rta||[]).length+'태스크';
}
function restoreOriginal(){
  if(!ORIGINAL.files){ const s=document.getElementById('bundleStatus'); if(s)s.innerHTML='❌ 스냅샷 없음'; return; }
  mutate(ALL_FILES,ORIGINAL.files.map(f=>({p:f.p,c:f.c})));
  clearObj(NOTES); Object.assign(NOTES,ORIGINAL.notes);
  mutate(PATH,ORIGINAL.path);
  mutate(QUIZ,ORIGINAL.quiz);
  mutate(SUBQUIZ,ORIGINAL.subquiz);
  mutate(MISSIONS,ORIGINAL.missions);
  mutate(MODULES,ORIGINAL.modules);
  rtaTasks=ORIGINAL.rta.map(t=>({...t}));
  state.progress={}; saveState();
  rebuildQuizMods();
  renderCodeSel(); renderPath(); renderQuiz(); renderSubQuiz(); renderMissions();
  renderRTATable(); calcRTA(); renderProgress();
  const s=document.getElementById('bundleStatus');
  if(s) s.innerHTML='↺ 원래 교재(FreeRTOS 프로젝트)로 복원했습니다.';
}
function rebuildQuizMods(){
  const sel=document.getElementById('quizMod');
  if(!sel) return;
  let h='<option value="all">전체</option>';
  PATH.forEach(m=>{ h+='<option value="'+esc(m.id)+'">'+esc(m.id)+'</option>'; });
  sel.innerHTML=h;
}
function loadManifestText(){
  const t=document.getElementById('manifestIn').value;
  const s=document.getElementById('bundleStatus');
  try{ loadBundle(JSON.parse(t)); }
  catch(e){ if(s) s.innerHTML='❌ JSON 파싱 오류: '+esc(e.message); }
}
function loadManifestFile(ev){
  const f=ev.target.files[0]; if(!f) return;
  const r=new FileReader();
  r.onload=function(){ try{ loadBundle(JSON.parse(r.result)); }catch(e){ const s=document.getElementById('bundleStatus'); if(s)s.innerHTML='❌ 오류: '+esc(e.message); } };
  r.readAsText(f);
}
function addSourceFiles(ev){
  const files=[].slice.call(ev.target.files);
  if(!files.length) return;
  const loaded=[];
  let remain=files.length;
  files.forEach(function(f){
    const r=new FileReader();
    r.onload=function(){
      loaded.push({path:f.name,content:r.result});
      remain--;
      if(remain===0){
        mutate(ALL_FILES,loaded);
        renderCodeSel();
        const s=document.getElementById('bundleStatus');
        if(s) s.innerHTML='📂 소스 파일 '+loaded.length+'개 로드 완료 — 학습 경로·퀴즈·미션은 기존 유지 (manifest 로 전체 교체 가능)';
      }
    };
    r.readAsText(f);
  });
}

const SAMPLE_BUNDLE=''' + sample_json + r''';
function loadSampleBundle(){ loadBundle(SAMPLE_BUNDLE); }

'''

if 'function loadBundle' not in html:
    html = html.rstrip()
    html = html.replace('</script>', JS_BLOCK + '\n</script>', 1)

# ===========================================================================
# D. 초기화: 스냅샷 캡처 (모든 const 선언 이후)
# ===========================================================================
if 'captureOriginal();' not in html:
    html = html.replace('</script>', 'captureOriginal();\n</script>', 1)

open(HTML, 'w', encoding='utf-8').write(html)
print('번들 로더 주입 완료:', len(html), 'bytes')

# ===========================================================================
# E. sample_bundle.json 덤프
# ===========================================================================
with open('/home/user/sample_bundle.json', 'w', encoding='utf-8') as fp:
    json.dump(SAMPLE, fp, ensure_ascii=False, indent=2)
print('sample_bundle.json 생성 완료')
