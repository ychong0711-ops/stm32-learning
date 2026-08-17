#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""make_build_main.py — 빌드용 main 변형 생성.
commented/main_fixed.c 에서 bsp 헤더와 중복되는 typedef 2개(CAN_Message_t, FirmwareState_t)를
제거한다. (README 의 통합 지침: 중복 typedef 삭제에 해당)"""
import os

SRC = '/home/user/commented/main_fixed.c'
DST_DIR = '/home/user/commented_build'
DST = os.path.join(DST_DIR, 'main_build.c')
os.makedirs(DST_DIR, exist_ok=True)

lines = open(SRC, encoding='utf-8').read().split('\n')

def find_block(end_sig, typedef_kw):
    end = None
    for i, l in enumerate(lines):
        if end_sig in l:
            end = i
            break
    if end is None:
        raise SystemExit('끝 시그니처를 찾지 못함: ' + end_sig)
    start = None
    for i in range(end, -1, -1):
        if lines[i].startswith('typedef ' + typedef_kw):
            start = i
            break
    if start is None:
        raise SystemExit('시작 typedef 를 찾지 못함: ' + end_sig)
    return start, end

removals = []
removals.append(find_block('} CAN_Message_t;', 'struct'))
removals.append(find_block('} FirmwareState_t;', 'enum'))

# 병합(정렬) 후 삭제
to_del = set()
for s, e in removals:
    for i in range(s, e + 1):
        to_del.add(i)

out = [l for i, l in enumerate(lines) if i not in to_del]
open(DST, 'w', encoding='utf-8').write('\n'.join(out))
print('생성:', DST, len(out), '줄 (원본', len(lines), '줄,', len(to_del), '줄 제거)')
