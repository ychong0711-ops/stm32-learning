#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""make_build_main.py — 빌드용 main 변형 생성.

원래 이 스크립트는 main_fixed.c 에서 bsp 헤더와 중복되는 typedef 2개
(CAN_Message_t, FirmwareState_t)를 잘라내는 일을 했다.
main_fixed.c 는 학습용으로 "혼자서도 읽히도록" 타입을 다시 적어 두었는데,
실제로 bsp 헤더와 같이 컴파일하면 재정의 오류가 나기 때문이다.

[2026-08 변경] 통합 작업에서 main_fixed.c 의 중복 typedef 를 아예 없앴다.
  - CAN_Message_t   → bsp/bsp_can.h 가 유일하게 정의한다.
  - FirmwareState_t → bsp/bootloader.h 가 유일하게 정의한다.
이제 원본이 그대로 컴파일되므로 잘라낼 것이 없다. 그래서 이 스크립트는
"있으면 지우고, 없으면 그냥 넘어가는" 멱등 동작으로 바뀌었다.
(예전에는 시그니처를 못 찾으면 SystemExit 로 죽었다)

경로는 저장소 기준으로 자동 계산하되, 인자로 덮어쓸 수 있다.
    python3 make_build_main.py [입력.c] [출력.c]
"""
import os
import sys

# 이 파일 위치에서 저장소 루트를 거슬러 올라가 기본 경로를 잡는다.
# (예전 버전은 /home/user/commented 라는 작성 당시 절대 경로에 묶여 있었다)
HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, '..', '..'))

SRC = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, '02_source_code', 'main_fixed.c')
DST = sys.argv[2] if len(sys.argv) > 2 else os.path.join(HERE, 'commented_build', 'main_build.c')

os.makedirs(os.path.dirname(DST), exist_ok=True)

if not os.path.exists(SRC):
    raise SystemExit('입력 파일이 없습니다: ' + SRC)

lines = open(SRC, encoding='utf-8').read().split('\n')


def find_block(end_sig, typedef_kw):
    """typedef 블록의 [시작, 끝] 줄 번호를 찾는다. 없으면 None 을 돌려준다."""
    end = None
    for i, l in enumerate(lines):
        if end_sig in l:
            end = i
            break
    if end is None:
        return None  # 이미 제거되었거나 애초에 없는 경우다. 정상으로 본다.

    start = None
    for i in range(end, -1, -1):
        if lines[i].startswith('typedef ' + typedef_kw):
            start = i
            break
    if start is None:
        # 끝은 있는데 시작이 없다면 파일이 예상과 다른 것이므로 조용히 넘기지 않는다.
        raise SystemExit('시작 typedef 를 찾지 못함: ' + end_sig)
    return start, end


targets = [
    ('} CAN_Message_t;', 'struct'),
    ('} FirmwareState_t;', 'enum'),
]

to_del = set()
removed_names = []
for end_sig, kw in targets:
    block = find_block(end_sig, kw)
    if block is None:
        continue
    start, end = block
    to_del.update(range(start, end + 1))
    removed_names.append(end_sig.strip('} ;'))

out = [l for i, l in enumerate(lines) if i not in to_del]
open(DST, 'w', encoding='utf-8').write('\n'.join(out))

if removed_names:
    print('생성:', DST, len(out), '줄 (원본', len(lines), '줄,',
          len(to_del), '줄 제거:', ', '.join(removed_names), ')')
else:
    print('생성:', DST, len(out), '줄 (중복 typedef 없음 — 원본을 그대로 복사)')
