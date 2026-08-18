#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# [보관용 — 실행하지 말 것]
# 이 HTML 을 만들어 온 일회성 UI 주입 스크립트다. 결과는 이미
# learning_program.html 에 반영되어 있고, 작성 당시의 절대 경로를
# 그대로 들고 있어 지금 실행하면 실패한다. 자세한 내용은 server/README.md 참조.
# 소스 스냅샷 갱신은 refresh_html_sources.py 를 쓴다.
"""
build_learning.py — learning_program.html 빌더 (멱등).
1) commented/ 의 모든 파일을 ALL_FILES 로 내장
2) 빌드 탭(문법 검사 + gcc + cppcheck + 통합 검사) 주입
3) MISRA-C 서브셋 휴리스틱 검사기 주입
재실행해도 중복 주입되지 않는다.
"""
import os, json, re

SRC = '/home/user/commented'
HTML = '/home/user/learning_program.html'

# ---------------------------------------------------------------------------
# 1) 파일 수집
# ---------------------------------------------------------------------------
def collect():
    files = []
    files.append(('main_fixed.c', open(os.path.join(SRC, 'main_fixed.c'), encoding='utf-8').read()))
    bsp_dir = os.path.join(SRC, 'bsp')
    for f in sorted(os.listdir(bsp_dir)):
        p = os.path.join(bsp_dir, f)
        if os.path.isfile(p):
            files.append(('bsp/' + f, open(p, encoding='utf-8').read()))
    for f in sorted(os.listdir(SRC)):
        if f.startswith('docs_'):
            files.append(('docs/' + f, open(os.path.join(SRC, f), encoding='utf-8').read()))
    files.append(('rta_analysis.py', open('/home/user/rta_analysis.py', encoding='utf-8').read()))
    return files

files = collect()
print('수집된 파일 수:', len(files))

entries = []
for path, content in files:
    entries.append('{p:%s,c:%s}' % (json.dumps(path), json.dumps(content)))
all_files_js = 'const ALL_FILES=[' + ',\n'.join(entries) + '];'

NOTES = {
    'main_fixed.c': '메인 애플리케이션(수정판). 워치독 전용 태스크(수정1)와 즉시 긴급정지 경로(수정2)가 포함된 전체 코드입니다.',
    'bsp/README.md': 'BSP 통합 가이드 — 핀맵, FreeRTOSConfig 설정, main_fixed.c 연동 시 수정사항이 정리되어 있습니다.',
    'bsp/bsp_can.c': 'CAN1 수신 — 인터럽트+링버퍼+세마포어 구조. ISR 안에 E2E 측정용 토글 핀이 내장되어 있습니다.',
    'bsp/bootloader.c': '부트로더 골격 — 플래싱 중 섹터 삭제 전·1KB 기록마다 IWDG를 피드해 벽돌을 방지합니다.',
    'bsp/bsp_iwdg.c': '독립 워치독 — LSI 기반, 타임아웃 계산 로직과 리셋 원인 조회(실험 E1~E4용)가 포함됩니다.',
    'bsp/bsp_pwm.c': 'TIM1 PWM — 20kHz 생성, 듀티 0~1000 퍼밀 단위. 어드밴스드 타이머의 BDTR 설정 포함.',
    'bsp/bsp_uart.c': 'USART2 + newlib _write() 리타겟 — printf가 115200bps로 출력됩니다.',
    'bsp/sensor_bmp280.c': 'BMP280 — I2C 통신과 데이터시트 온도 보상 공식(int32 정수 연산) 구현.',
    'bsp/system_stm32.c': '시스템 클록 설정(HSE 8MHz → PLL 180MHz)과 NVIC 그룹 설정 래퍼.',
    'rta_analysis.py': '응답시간 분석 계산기(파이썬) — Liu–Layland/Bini 상한과 고정점 반복 RTA를 실제로 실행합니다.',
    'docs/docs_realtime_theory_proof.md': '실시간 이론 수학적 증명 — 정리 4개 증명 스케치 + RTA 방정식 유도 + 태스크셋 판정.',
    'docs/docs_measurement_plan.md': '실측 계획서 — 스택/CPU부하/지터/E2E/워치독 5개 측정의 방법론과 합격 기준.',
    'docs/docs_measurement_report_template.md': '실측 보고서 템플릿 — 측정 후 결과를 채워 넣을 양식.',
    'docs/docs_autosar_iso26262.md': 'AUTOSAR·ISO 26262 개념 요약 — 프로젝트와의 대응표 포함.',
    'docs/docs_motivation_letter.md': '지원 동기서 초안(영문 제출용)과 작성 전략.',
}
notes_js = 'const NOTES=' + json.dumps(NOTES, ensure_ascii=False) + ';'

html = open(HTML, encoding='utf-8').read()

# ---------------------------------------------------------------------------
# 2) EXCERPTS → ALL_FILES 교체 (멱등)
# ---------------------------------------------------------------------------
if 'const ALL_FILES=' not in html:
    new_block = all_files_js + '\n\n' + notes_js + '''

function renderCodeSel(){
  const sel=document.getElementById('codeSel');
  sel.innerHTML='';
  ALL_FILES.forEach((f,i)=>{
    const o=document.createElement('option'); o.value=i;
    const dir=f.p.split('/').length>1 ? f.p.split('/')[0]+'/' : '루트';
    o.textContent=dir+'  '+f.p;
    sel.appendChild(o);
  });
  sel.onchange=showCode;
  showCode();
}

function renderLine(l,i){
  const ci=l.indexOf('//');
  if(ci>=0) return '<span class="ln">'+(i+1)+'</span>'+esc(l.slice(0,ci))+'<span class="cmt">'+esc(l.slice(ci))+'</span>';
  const t=l.trim();
  if(t.startsWith('#') && !t.startsWith('#include')) return '<span class="ln">'+(i+1)+'</span><span class="cmt">'+esc(l)+'</span>';
  return '<span class="ln">'+(i+1)+'</span>'+esc(l);
}

function showCode(){
  const f=ALL_FILES[document.getElementById('codeSel').value];
  const lines=f.c.split('\\n');
  const view=document.getElementById('codeView');
  view.innerHTML=lines.map(renderLine).join('\\n');
  const note=NOTES[f.p]||'이 파일의 한 줄씩 주석을 읽어보세요.';
  document.getElementById('codeNote').textContent='📄 '+f.p+' — '+lines.length+'줄 · 💡 '+note;
}

renderCodeSel();'''
    pat = re.compile(r'const EXCERPTS=\[.*?renderCodeSel\(\);', re.S)
    html, n = pat.subn(lambda m: new_block, html, count=1)
    if n == 0:
        raise SystemExit('EXCERPTS 블록을 찾지 못했습니다')
    print('EXCERPTS → ALL_FILES 교체 완료')
else:
    print('ALL_FILES 주입은 이미 완료됨 — 건너뜀')

# 코드 탭 설명문 교체 (멱등)
html = html.replace(
    '<p class="dim">전 줄 한글 주석이 달린 핵심 코드를 선택해 읽으세요. 코드의 모든 줄에 주석이 있다는 점이 이 교재의 특징입니다.</p>',
    '<p class="dim">워크스페이스의 <b>전체 29개 파일</b>(main_fixed.c · bsp/ 22개 · docs/ 5개 · rta_analysis.py)을 내장했습니다. 드롭다운에서 파일을 선택해 읽으세요. 모든 코드 줄에 한글 주석이 있습니다.</p>'
)

# ---------------------------------------------------------------------------
# 3) 빌드 탭 주입 (멱등)
# ---------------------------------------------------------------------------
if 'data-s="build"' not in html:
    html = html.replace(
        '    <button data-s="quiz">📝 퀴즈</button>',
        '    <button data-s="quiz">📝 퀴즈</button>\n    <button data-s="build">🔨 빌드</button>'
    )

if 'section id="build"' not in html:
    build_section = '''<!-- ============ BUILD ============ -->
<section id="build">
  <h1>🔨 빌드 (컴파일 체크)</h1>
  <p class="dim">검사 3종 + 통합 검사를 제공합니다. <b>① 오프라인 문법/규칙 검사</b>는 브라우저에서 즉시 동작합니다. <b>② gcc</b>는 STM32 HAL/FreeRTOS 스텁 헤더로 <code>-fsyntax-only</code>, <b>③ cppcheck</b>는 정적 분석을 수행합니다. ②③은 라이브 프리뷰에서만 동작합니다.</p>
  <div class="card">
    <button class="act" onclick="loadSample('bad')">예시 ① 결함 있는 원본</button>
    <button class="act" onclick="loadSample('good')">예시 ② 수정본</button>
    <button class="ghost" onclick="document.getElementById('buildIn').value=''">비우기</button>
  </div>
  <textarea id="buildIn" placeholder="// 컴파일 체크할 C 코드를 붙여넣으세요... (include 없이 함수 단위도 가능)"></textarea>
  <div>
    <button class="act" onclick="clientBuild()">① 오프라인 문법 검사</button>
    <button class="act" onclick="gccBuild()">② gcc 컴파일 체크 (서버)</button>
    <button class="act" onclick="cppcheckBuild()">③ cppcheck 정적 분석 (서버)</button>
    <button class="ghost" onclick="projectBuild()">🔧 프로젝트 전체 체크 (서버)</button>
    <button class="act" style="background:var(--acc2)" onclick="fullCheck('buildIn','buildOut')">🚀 통합 검사 (문법+규칙+MISRA+gcc+cppcheck)</button>
  </div>
  <div id="buildOut"></div>
</section>

'''
    html = html.replace('<div class="foot">', build_section + '<div class="foot">')

# 리뷰 탭에 연동 검사 버튼 추가 (멱등)
html = html.replace(
    '<div><button class="act" onclick="runReview()">🔍 검사 실행</button></div>',
    '<div><button class="act" onclick="runReview()">🔍 검사 실행</button>\n  <button class="ghost" onclick="fullCheck(\'codeIn\',\'reviewOut\')">🔗 gcc·cppcheck 연동 검사</button></div>'
)

# 기존(구버전) 빌드 탭 설명/버튼을 3종+통합으로 확장 (멱등)
html = html.replace(
    '<p class="dim">두 가지 검사를 제공합니다.',
    '<p class="dim">검사 3종 + 통합 검사를 제공합니다.'
)
if 'cppcheckBuild()' not in html:
    html = html.replace(
        '    <button class="act" onclick="gccBuild()">② gcc 컴파일 체크 (서버)</button>\n    <button class="ghost" onclick="projectBuild()">🔧 프로젝트 전체 체크 (서버)</button>',
        '    <button class="act" onclick="gccBuild()">② gcc 컴파일 체크 (서버)</button>\n    <button class="act" onclick="cppcheckBuild()">③ cppcheck 정적 분석 (서버)</button>\n    <button class="ghost" onclick="projectBuild()">🔧 프로젝트 전체 체크 (서버)</button>\n    <button class="act" style="background:var(--acc2)" onclick="fullCheck(\'buildIn\',\'buildOut\')">🚀 통합 검사 (문법+규칙+MISRA+gcc+cppcheck)</button>'
    )

html = html.replace(
    "function loadSample(k){ document.getElementById('codeIn').value=SAMPLES[k]; }",
    "function loadSample(k){ document.getElementById('codeIn').value=SAMPLES[k]; const b=document.getElementById('buildIn'); if(b) b.value=SAMPLES[k]; }"
)

html = html.replace(
    '<li><b>퀴즈</b> — 18문항 자동 채점 + 해설</li>',
    '<li><b>퀴즈</b> — 18문항 자동 채점 + 해설</li>\n        <li><b>빌드</b> — 문법 검사 + gcc·cppcheck·MISRA 통합 검사</li>'
)

# ---------------------------------------------------------------------------
# 4) 빌드 JS 주입 (멱등) — 문법 검사 + gcc + cppcheck + 통합 검사
# ---------------------------------------------------------------------------
if 'function clientSyntaxCheck' not in html:
    build_js = '''/* ============================================================
   빌드 — 오프라인 문법 검사 + gcc/cppcheck/통합 검사
============================================================ */
function clientSyntaxCheck(c){
  const issues=[];
  let line=1, inBlock=false;
  const open={'{':'}','(':')','[':']'};
  const stack=[];
  let i=0; const n=c.length;
  while(i<n){
    const ch=c[i], nx=c[i+1];
    if(ch==='\\n'){ line++; i++; continue; }
    if(inBlock){ if(ch==='*'&&nx==='/'){ inBlock=false; i+=2; } else i++; continue; }
    if(ch==='/'&&nx==='/'){ while(i<n&&c[i]!=='\\n') i++; continue; }
    if(ch==='/'&&nx==='*'){ inBlock=true; i+=2; continue; }
    if(ch==='"'||ch==="'"){ const q=ch; i++; while(i<n&&c[i]!==q){ if(c[i]==='\\\\')i++; i++; } if(i<n)i++; continue; }
    if(ch==='{'||ch==='('||ch==='['){ stack.push({ch,line}); i++; continue; }
    if(ch==='}'||ch===')'||ch===']'){
      if(stack.length===0){ issues.push({line,msg:'닫는 괄호 '+ch+' 에 대응하는 여는 괄호가 없음'}); i++; continue; }
      const top=stack.pop();
      if(open[top.ch]!==ch){ issues.push({line,msg:'괄호 불일치 — '+top.line+'행의 '+top.ch+' 가 '+ch+' 로 닫힘'}); }
      i++; continue;
    }
    i++;
  }
  if(inBlock) issues.push({line,msg:'블록 주석(/* ... */)이 닫히지 않음'});
  stack.forEach(s=>issues.push({line:s.line,msg:'닫히지 않은 '+s.ch}));
  c.split('\\n').forEach((l,idx)=>{ if(/^\\s*;\\s*$/.test(l)) issues.push({line:idx+1,msg:'빈 문장(;) — 에러 처리가 누락된 자리일 수 있음'}); });
  issues.sort((a,b)=>a.line-b.line);
  return issues;
}
function clientBuild(){
  const c=document.getElementById('buildIn').value;
  const out=document.getElementById('buildOut');
  const issues=clientSyntaxCheck(c);
  let html='<div class="card"><h2>① 오프라인 문법 검사</h2>';
  if(issues.length===0){ html+='<div class="finding info"><div class="t">✅ 문법 구조 이상 없음</div><div class="dim">괄호·주석·빈 문장 검사 통과. (의미·타입 검사는 ② gcc 체크를 사용하세요)</div></div>'; }
  else{
    html+='<p class="dim">'+issues.length+'건 발견</p>';
    issues.forEach(it=>{ html+='<div class="finding warn"><div class="t">'+it.line+'행</div><div class="dim">'+esc(it.msg)+'</div></div>'; });
  }
  html+='</div>'; out.innerHTML=html;
}
function renderBuildResult(ok,output,label){
  const out=document.getElementById('buildOut');
  const t=esc(output||'(진단 없음)');
  out.innerHTML='<div class="card"><h2>'+(ok?'✅ 빌드 성공':'❌ 컴파일 오류')+' — '+esc(label)+'</h2><pre class="code" style="color:'+(ok?'var(--acc2)':'var(--crit)')+'">'+t+'</pre></div>';
}
async function gccBuild(){
  const code=document.getElementById('buildIn').value;
  const out=document.getElementById('buildOut');
  out.innerHTML='<div class="card dim">gcc -fsyntax-only 검사 중...</div>';
  try{
    const r=await fetch('/api/build',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({code})});
    const j=await r.json(); renderBuildResult(j.ok,j.output,'gcc -fsyntax-only (스텁 헤더 포함)');
  }catch(e){
    out.innerHTML='<div class="card"><div class="finding warn"><div class="t">빌드 서버에 연결할 수 없습니다</div><div class="dim">gcc 컴파일 체크는 라이브 프리뷰에서 실행하세요. 정적 미리보기에서는 ① 오프라인 문법 검사만 동작합니다.</div></div></div>';
  }
}
async function projectBuild(){
  const out=document.getElementById('buildOut');
  out.innerHTML='<div class="card dim">프로젝트 전체 파일을 컴파일 검사 중...</div>';
  try{
    const r=await fetch('/api/build-project',{method:'POST'});
    const j=await r.json(); renderBuildResult(j.ok,j.output,'전체 프로젝트 (main_build.c + bsp/*.c)');
  }catch(e){
    out.innerHTML='<div class="card"><div class="finding warn"><div class="t">빌드 서버에 연결할 수 없습니다</div><div class="dim">라이브 프리뷰에서 실행하세요.</div></div></div>';
  }
}

'''
    html = html.replace('/* ============================================================\n   퀴즈', build_js + '/* ============================================================\n   퀴즈')

# ---------------------------------------------------------------------------
# 5) MISRA-C 서브셋 + cppcheck + 통합 검사 주입 (멱등)
# ---------------------------------------------------------------------------
if 'function clientMisraCheck' not in html:
    misra_js = '''/* ============================================================
   MISRA-C 2012 서브셋 (휴리스틱) + cppcheck + 통합 검사
   ※ 정식 MISRA 준수 인증은 라이선스된 misra.json 필요. 여기는 교육용 서브셋.
============================================================ */
function clientMisraCheck(c){
  const issues=[];
  const lines=c.split('\\n');
  // M21.x — 금지/제한 표준 라이브러리
  const banned=/\\b(malloc|calloc|realloc|free|sprintf|vsprintf|strcpy|strcat|gets|scanf|sscanf|atoi|atof|rand|srand)\\s*\\(/g;
  lines.forEach((l,i)=>{ banned.lastIndex=0; let m; while((m=banned.exec(l))) issues.push({line:i+1,id:'M21.x',msg:'제한된 표준 함수 "'+m[1]+'" 사용 — 동적 할당·비안전 문자열 함수는 MISRA에서 금지(예: M21.6, M21.8)'}); });
  // M20.2 — 예약 식별자
  lines.forEach((l,i)=>{ if(/\\b__[A-Za-z_]|\\b_[A-Z]\\w*/.test(l)) issues.push({line:i+1,id:'M20.2',msg:'예약 식별자 가능성 — 밑줄 2개(__) 또는 밑줄+대문자 시작 식별자는 컴파일러/표준 예약 (단, 벤더 HAL 매크로는 예외)'}); });
  // M16.7 — 매직 넘버
  lines.forEach((l,i)=>{ const clean=l.replace(/(["'])(?:\\\\.|(?!\\1).)*\\1/g,'').replace(/\\/\\/.*/,''); const mm=clean.match(/\\b(?:0x[0-9A-Fa-f]+|\\d+)\\b/g); if(mm){ const bad=mm.filter(x=>!/^(0|1)$/.test(x)&&!/^0x[01]$/i.test(x)); if(bad.length) issues.push({line:i+1,id:'M16.7',msg:'매직 넘버: '+[...new Set(bad)].join(', ')+' — 명명된 상수(#define/enum)로 대체 권장'}); } });
  // M16.4 — switch default
  const swRe=/\\bswitch\\s*\\(/g; let sm;
  while((sm=swRe.exec(c))){
    const k=c.indexOf('{',sm.index); if(k<0){continue;}
    let depth=0,end=-1;
    for(let j=k;j<c.length;j++){ if(c[j]==='{')depth++; else if(c[j]==='}'){depth--; if(depth===0){end=j;break;}} }
    if(end<0) continue;
    if(!/\\bdefault\\s*:/.test(c.slice(k,end))){ issues.push({line:c.slice(0,sm.index).split('\\n').length,id:'M16.4',msg:'switch 문에 default 절이 없음'}); }
    swRe.lastIndex=end+1;
  }
  // M15.7 — else-if 사슬에 마지막 else 없음
  if(/\\belse\\s+if\\s*\\(/.test(c)){
    const elseCount=(c.match(/\\belse\\b/g)||[]).length;
    const eiCount=(c.match(/\\belse\\s+if\\s*\\(/g)||[]).length;
    if(elseCount===eiCount){ lines.forEach((l,i)=>{ if(/\\belse\\s+if\\s*\\(/.test(l)) issues.push({line:i+1,id:'M15.7',msg:'if-else if 사슬에 마지막 else 가 없음 — 모든 경로를 명시 처리 권장'}); }); }
  }
  // M15.6 — 제어문 본문 중괄호
  lines.forEach((l,i)=>{ if(/\\b(if|else|while|for)\\s*\\([^)]*\\)\\s*[^;{\\s]/.test(l)) issues.push({line:i+1,id:'M15.6',msg:'제어문 본문이 중괄호로 묶이지 않음 — 복합문 사용 권장'}); });
  for(let i=1;i<lines.length;i++){ if(/\\b(if|else\\s+if|while|for)\\s*\\([^)]*\\)\\s*$/.test(lines[i-1].trim()) && !/^\\s*\\{/.test(lines[i])) issues.push({line:i,id:'M15.6',msg:'제어문 본문이 중괄호로 묶이지 않음 (다음 줄에 { 없음)'}); }
  // M17.2 — 재귀 호출
  const fdef=/\\b([A-Za-z_]\\w*)\\s*\\([^;{}]*\\)\\s*\\{/g; let fm;
  while((fm=fdef.exec(c))){
    const name=fm[1]; const k=c.indexOf('{',fm.index); let depth=0,end=-1;
    for(let j=k;j<c.length;j++){ if(c[j]==='{')depth++; else if(c[j]==='}'){depth--; if(depth===0){end=j;break;}} }
    if(end<0) continue;
    if(new RegExp('\\\\b'+name+'\\\\s*\\\\(').test(c.slice(k,end))) issues.push({line:c.slice(0,fm.index).split('\\n').length,id:'M17.2',msg:'재귀 호출 감지("'+name+'"가 자신을 호출) — MISRA는 재귀 금지'}); 
    fdef.lastIndex=end+1;
  }
  issues.sort((a,b)=>a.line-b.line);
  return issues;
}
async function cppcheckBuild(){
  const code=document.getElementById('buildIn').value;
  const out=document.getElementById('buildOut');
  out.innerHTML='<div class="card dim">cppcheck 분석 중...</div>';
  try{
    const r=await fetch('/api/cppcheck',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({code})});
    const j=await r.json();
    if(!j.available){ out.innerHTML='<div class="card"><div class="finding warn"><div class="t">cppcheck 미설치</div><div class="dim">'+esc(j.note||'')+'</div></div></div>'; return; }
    if(!j.output){ out.innerHTML='<div class="card"><h2>✅ cppcheck — 이슈 없음</h2><p class="dim">warning·style·performance·portability 검사에서 발견된 이슈가 없습니다.</p></div>'; return; }
    out.innerHTML='<div class="card"><h2>cppcheck 정적 분석 결과</h2><pre class="code" style="color:var(--warn)">'+esc(j.output)+'</pre><p class="dim">정식 MISRA-C 준수 인증은 라이선스된 misra.json 이 필요합니다. 여기서는 cppcheck 내장 검사 + ⑤ MISRA 서브셋(휴리스틱)을 제공합니다.</p></div>';
  }catch(e){
    out.innerHTML='<div class="card"><div class="finding warn"><div class="t">서버 연결 실패</div><div class="dim">라이브 프리뷰에서 실행하세요.</div></div></div>';
  }
}
async function fullCheck(srcId,outId){
  const c=document.getElementById(srcId).value;
  const out=document.getElementById(outId);
  if(!c.trim()){ out.innerHTML='<div class="card dim">검사할 코드가 없습니다.</div>'; return; }
  out.innerHTML='<div class="card dim">🚀 통합 검사 실행 중... (문법 → 결함 규칙 → MISRA → gcc → cppcheck)</div>';
  const syntax=clientSyntaxCheck(c);
  const rev=RULES.filter(r=>r.test(c));
  const misra=clientMisraCheck(c);
  let gcc={done:false,out:''}, cpp={done:false,out:'',avail:true};
  try{ const j=await (await fetch('/api/build',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({code:c})})).json(); gcc={done:true,out:j.output}; }
  catch(e){ gcc={done:false,out:''}; }
  try{ const j=await (await fetch('/api/cppcheck',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({code:c})})).json(); if(!j.available){cpp={done:true,avail:false,out:j.note||''};} else {cpp={done:true,avail:true,out:j.output};} }
  catch(e){ cpp={done:false,avail:true,out:''}; }
  const sev=s=>s==='crit'?'치명':s==='warn'?'경고':'정보';
  let h='<div class="card"><h2>🚀 통합 검사 보고서</h2>';
  h+='<h2>① 문법 구조 ('+syntax.length+')</h2>';
  h+=syntax.length?syntax.map(it=>'<div class="finding warn"><div class="t">'+it.line+'행</div><div class="dim">'+esc(it.msg)+'</div></div>').join(''):'<div class="finding info"><div class="t">✅ 이상 없음</div></div>';
  h+='<h2>② 결함 패턴 규칙 ('+rev.length+')</h2>';
  h+=rev.length?rev.map(r=>'<div class="finding '+r.sev+'"><div class="t">'+r.t+' <span class="pill '+r.sev+'">'+sev(r.sev)+'</span> <span class="dim">('+r.id+')</span></div><div class="dim">'+r.m+'</div><div class="dim" style="color:var(--acc2)">수정: '+r.f+'</div></div>').join(''):'<div class="finding info"><div class="t">✅ 발견 없음</div></div>';
  h+='<h2>③ MISRA-C 서브셋 ('+misra.length+')</h2>';
  h+=misra.length?misra.map(it=>'<div class="finding warn"><div class="t">'+it.id+' · '+it.line+'행</div><div class="dim">'+esc(it.msg)+'</div></div>').join(''):'<div class="finding info"><div class="t">✅ 발견 없음</div></div>';
  h+='<h2>④ gcc 컴파일 체크</h2>';
  if(!gcc.done){ h+='<div class="finding warn"><div class="t">서버 연결 실패</div><div class="dim">라이브 프리뷰에서 실행하세요.</div></div>'; }
  else if(!gcc.out){ h+='<div class="finding info"><div class="t">✅ 컴파일 통과 (경고·오류 0)</div></div>'; }
  else { h+='<div class="finding crit"><div class="t">컴파일 오류/경고</div><pre class="code">'+esc(gcc.out)+'</pre></div>'; }
  h+='<h2>⑤ cppcheck 정적 분석</h2>';
  if(cpp.avail===false){ h+='<div class="finding warn"><div class="t">cppcheck 미설치</div><div class="dim">'+esc(cpp.out)+'</div></div>'; }
  else if(!cpp.done){ h+='<div class="finding warn"><div class="t">서버 연결 실패</div><div class="dim">라이브 프리뷰에서 실행하세요.</div></div>'; }
  else if(!cpp.out){ h+='<div class="finding info"><div class="t">✅ 이슈 없음</div></div>'; }
  else { h+='<div class="finding warn"><div class="t">이슈 발견</div><pre class="code">'+esc(cpp.out)+'</pre></div>'; }
  h+='</div>';
  out.innerHTML=h;
}

'''
    html = html.replace('/* ============================================================\n   퀴즈', misra_js + '/* ============================================================\n   퀴즈')

open(HTML, 'w', encoding='utf-8').write(html)
print('HTML 재생성 완료:', len(html), 'bytes')
