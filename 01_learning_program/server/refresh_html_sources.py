#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
refresh_html_sources.py — learning_program.html 안에 내장된 소스 스냅샷 갱신.

learning_program.html 은 학습용 단일 파일 앱이라 소스를 `const ALL_FILES=[...]`
JSON 배열로 내부에 들고 있다. 저장소의 소스를 고치면 이 스냅샷이 옛날 것으로
남기 때문에, 이 스크립트로 다시 찍어 넣는다.

- ALL_FILES 와 NOTES 두 선언만 갈아끼운다. UI 코드는 건드리지 않는다.
- 멱등하다. 몇 번을 돌려도 결과가 같다.
- 저장소 위치는 스크립트 경로에서 계산한다. 절대경로 하드코딩 없음.

사용법:
    python3 refresh_html_sources.py             # 제자리 갱신
    python3 refresh_html_sources.py --check     # 갱신 필요 여부만 보고 (CI용)
"""
import argparse
import json
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))          # .../stm32-learning
SRC = os.path.join(REPO, '02_source_code')
HTML = os.path.join(REPO, '01_learning_program', 'learning_program.html')

# 내장할 파일 목록. (표시 경로, 실제 경로) 순서가 그대로 UI 파일 목록 순서가 된다.
def collect():
    """저장소에서 학습용으로 보여줄 파일을 모은다."""
    out = []

    def add(display, real):
        if os.path.isfile(real):
            with open(real, encoding='utf-8') as f:
                out.append((display, f.read()))
        else:
            print('  (없음, 건너뜀) %s' % display, file=sys.stderr)

    add('main_fixed.c', os.path.join(SRC, 'main_fixed.c'))
    add('Makefile', os.path.join(SRC, 'Makefile'))
    add('BUILD.md', os.path.join(SRC, 'BUILD.md'))

    for sub in ('bsp', 'app', 'bootloader', 'linker', 'tools'):
        d = os.path.join(SRC, sub)
        if not os.path.isdir(d):
            continue
        for name in sorted(os.listdir(d)):
            p = os.path.join(d, name)
            if os.path.isfile(p) and not name.endswith(('.o', '.d', '.su')):
                add('%s/%s' % (sub, name), p)

    docs = os.path.join(SRC, 'docs')
    if os.path.isdir(docs):
        for name in sorted(os.listdir(docs)):
            if name.endswith('.md'):
                add('docs/' + name, os.path.join(docs, name))

    add('rta_analysis.py', os.path.join(SRC, 'rta_analysis.py'))
    return out


# 파일별 한 줄 설명. 목록에 없으면 UI 가 설명 없이 보여준다.
NOTES = {
    'main_fixed.c':
        '메인 애플리케이션(수정판). 워치독 전용 태스크(수정1)와 즉시 긴급정지 '
        '경로(수정2)가 포함된 전체 코드입니다. 펌웨어 태스크는 이제 직접 '
        '플래싱하지 않고 스테이징 이미지를 검증한 뒤 부트로더에 설치를 요청합니다.',
    'Makefile':
        '부트로더와 앱을 각각 다른 링커 스크립트·최적화 옵션으로 빌드하는 '
        '2단 이미지 빌드 시스템. `make deps` 로 벤더 소스를 받고 `make` 로 전부 만듭니다.',
    'BUILD.md':
        '빌드 방법과 컴파일 플래그의 이유, 안 될 때 확인할 것들. '
        '왜 -nostdlib 로 링크하는지도 여기 있습니다.',

    'bsp/README.md':
        'BSP 통합 가이드 — 핀맵, FreeRTOSConfig 설정, main_fixed.c 연동 시 수정사항.',
    'bsp/flash_map.h':
        '플래시 영역 배치도. 섹터 0=부트로더, 1~4=앱, 5=스테이징. '
        '백업 SRAM 의 부트 제어 블록(BootCtrl_t) 정의도 여기 있습니다.',
    'bsp/fw_image.h':
        '펌웨어 이미지 32바이트 헤더 정의. 매직·크기·본문 CRC·헤더 CRC.',
    'bsp/fw_image.c':
        'CRC-32(IEEE) 계산과 이미지 검증. 헤더 CRC 를 본문 CRC 와 분리한 이유는 '
        '크기 필드를 믿기 전에 헤더부터 검증해야 하기 때문입니다.',
    'bsp/bootloader.c':
        '앱 쪽 스테이징 계층 — 수신 이미지를 섹터 5 에 쌓고, 검증하고, '
        '설치를 요청한 뒤 리셋합니다. 자기 자신을 덮어쓰지 않으므로 벽돌 구간이 없습니다.',
    'bsp/bootloader.h':
        '스테이징 API. 옛 Bootloader_FlashNewFirmware() 는 삭제되었습니다.',
    'bsp/bsp_can.c':
        'CAN1 수신 — 인터럽트+링버퍼+세마포어 구조. ISR 안에 E2E 측정용 토글 핀 내장.',
    'bsp/bsp_iwdg.c':
        '독립 워치독 — LSI 기반, 타임아웃 계산과 리셋 원인 조회(실험 E1~E4용).',
    'bsp/bsp_flash.c':
        '내부 플래시 삭제/기록. 섹터 단위 소거, 워드 단위 프로그래밍.',
    'bsp/bsp_baremetal.h':
        '부트로더용 최소 타입 정의. 부트로더는 FreeRTOS 없이 도니까 '
        'BaseType_t 같은 것만 따로 제공합니다.',

    'bootloader/boot_main.c':
        '섹터 0 부트로더 본체. 검증 → 설치 → 점프. 점프 전에 클록·인터럽트·SysTick 을 '
        '전부 리셋 직후 상태로 되돌리고 VTOR 을 옮깁니다. 이 순서가 틀리면 앱이 이상하게 죽습니다.',

    'app/FreeRTOSConfig.h':
        'FreeRTOS 설정. SysTick_Handler 는 우리가 직접 정의하므로 '
        'xPortSysTickHandler 매핑을 일부러 뺐습니다(HAL_IncTick 도 같이 불러야 함).',
    'app/stm32f4xx_it.c':
        '인터럽트 진입점. 이게 없어서 원래 프로젝트는 링크조차 되지 않았습니다.',
    'app/tiny_printf.c':
        'newlib 없이 쓰는 정수 전용 printf. 스택 사용량이 정적으로 계산 가능해야 '
        '해서 표준 라이브러리를 안 씁니다.',
    'app/tiny_libc.c':
        'memcpy/memset 등 컴파일러가 필요로 하는 최소 libc.',

    'linker/STM32F446RE_APP.ld':
        '앱 링커 스크립트 — FLASH 는 0x08004000 부터 112KB.',
    'linker/STM32F446RE_BOOT.ld':
        '부트로더 링커 스크립트 — FLASH 는 0x08000000 부터 16KB.',

    'tools/make_image.py':
        '앱 바이너리 앞에 32바이트 헤더를 붙여 갱신용 이미지를 만듭니다. '
        '벡터 테이블이 말이 되는지도 여기서 한 번 검사합니다.',
    'tools/make_combined.py':
        '부트로더와 앱을 하나의 .bin 으로 합칩니다. 공장 출하/첫 굽기용.',

    'docs/docs_realtime_theory_proof.md':
        '실시간 이론 증명 — RMS/RTA 로 스케줄 가능성을 계산합니다.',
    'docs/docs_measurement_plan.md':
        '실측 계획서 — 무엇을 어떻게 잴 것인지.',
    'docs/docs_bootloader_design.md':
        '부트로더·스테이징 설계 문서. 왜 셀프 플래싱을 버렸는지, '
        '메모리를 왜 이렇게 나눴는지, 점프 절차의 각 단계가 왜 필요한지.',
    'docs/docs_autosar_iso26262.md':
        'AUTOSAR·ISO 26262 개념과 이 프로젝트에의 대응.',
    'docs/docs_measurement_report_template.md':
        '측정 보고서 양식.',
    'docs/docs_motivation_letter.md':
        '지원 동기서.',

    'rta_analysis.py':
        'RTA(응답시간 분석) 계산기. 문서의 숫자를 이걸로 뽑습니다.',
}


def find_decl(text, name):
    """`const NAME=` 선언의 값 범위를 (시작, 끝) 으로 돌려준다.

    괄호 짝을 세어 찾는다. 문자열 리터럴 안의 괄호는 무시한다.
    """
    marker = 'const %s=' % name
    i = text.find(marker)
    if i < 0:
        raise SystemExit('learning_program.html 에서 %s 선언을 찾지 못했습니다.' % name)
    k = i + len(marker)
    opener = text[k]
    closer = {'[': ']', '{': '}'}[opener]
    depth = 0
    instr = False
    esc = False
    p = k
    while p < len(text):
        ch = text[p]
        if instr:
            if esc:
                esc = False
            elif ch == '\\':
                esc = True
            elif ch == '"':
                instr = False
        else:
            if ch == '"':
                instr = True
            elif ch == opener:
                depth += 1
            elif ch == closer:
                depth -= 1
                if depth == 0:
                    return i, p + 1
        p += 1
    raise SystemExit('%s 선언이 닫히지 않았습니다.' % name)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--check', action='store_true',
                    help='갱신하지 않고 최신인지만 확인 (다르면 종료코드 1)')
    ap.add_argument('--html', default=HTML, help='대상 HTML 경로')
    args = ap.parse_args()

    files = collect()
    print('수집된 파일 수: %d' % len(files))

    entries = ['{p:%s,c:%s}' % (json.dumps(p), json.dumps(c)) for p, c in files]
    all_files_js = 'const ALL_FILES=[' + ',\n'.join(entries) + ']'

    used = {p: NOTES[p] for p, _ in files if p in NOTES}
    notes_js = 'const NOTES=' + json.dumps(used, ensure_ascii=False)

    with open(args.html, encoding='utf-8') as f:
        html = f.read()

    # 뒤에서부터 바꿔야 앞쪽 인덱스가 밀리지 않는다.
    a0, a1 = find_decl(html, 'ALL_FILES')
    n0, n1 = find_decl(html, 'NOTES')
    for start, end, new in sorted([(a0, a1, all_files_js), (n0, n1, notes_js)],
                                  reverse=True):
        html = html[:start] + new + html[end:]

    with open(args.html, encoding='utf-8') as f:
        before = f.read()

    if html == before:
        print('이미 최신입니다.')
        return 0

    if args.check:
        print('스냅샷이 저장소 소스와 다릅니다. '
              'refresh_html_sources.py 를 --check 없이 실행하세요.', file=sys.stderr)
        return 1

    with open(args.html, 'w', encoding='utf-8') as f:
        f.write(html)
    print('갱신 완료: %s (%d bytes -> %d bytes)'
          % (args.html, len(before), len(html)))
    print('설명이 붙은 파일: %d / %d' % (len(used), len(files)))
    return 0


if __name__ == '__main__':
    sys.exit(main())
