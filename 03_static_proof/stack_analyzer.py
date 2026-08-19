#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
stack_analyzer.py — GCC -fcallgraph-info(.ci) + -fstack-usage(.su) 기반
태스크별 최악 스택 사용량 계산기 (하드웨어 불필요, 정적 분석)

원리:
  .su 파일 → 함수별 프레임 크기(바이트)
  .ci 파일 → 함수 호출 그래프 (edge)
  최악 스택(루트) = max( 프레임(루트) + Σ 최악스택(자식) )  ← DAG 상의 최장 경로
  재귀/외부 미확인 심볼은 "외부 호출"로 표기하고, 사이클은 DFS에서 제외(재귀 검출).
"""
import re, sys
from collections import defaultdict

def parse_su(path):
    frames = {}
    for line in open(path, encoding='utf-8'):
        line = line.strip()
        if not line or '\t' not in line:
            continue
        head, size, qual = line.split('\t')
        func = head.split(':')[-1]           # "path:line:col:func"
        if qual == 'dynamic':
            frames[func] = None               # 동적 스택 → 크기 미상 (보수적으로 ?)
        else:
            frames[func] = int(size)
    return frames

def parse_ci(path):
    edges = defaultdict(set)
    for line in open(path, encoding='utf-8'):
        m = re.search(r'sourcename:\s*"([^"]+)"\s+targetname:\s*"([^"]+)"', line)
        if m:
            edges[m.group(1)].add(m.group(2))
    return edges

def worst_stack(func, frames, edges, memo, stack_path):
    """DAG 최장 경로로 최악 스택 계산 (재귀 사이클 방지)"""
    if func in memo:
        return memo[func]
    if func in stack_path:                    # 재귀 → 사이클
        memo[func] = (None, 'recursive')
        return memo[func]
    f = frames.get(func)
    if f is None:
        memo[func] = (0, 'external')   # 외부 함수 프레임은 미상 → 0 으로 두고 하한만 계산
        return memo[func]
    best = 0
    for child in edges.get(func, []):
        cs, why = worst_stack(child, frames, edges, memo, stack_path | {func})
        best = max(best, cs)
    memo[func] = (f + best, None)
    return memo[func]

def main():
    roots = sys.argv[1:]                      # 예: vTask_Watchdog vTask_CAN_Rx ...
    frames = {}
    edges = defaultdict(set)
    import glob, os
    # 실제 산출물 위치: analysis_results/ (예전 build_su/ build_ci/ 경로가 아님)
    base = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'analysis_results')
    su_files = glob.glob(os.path.join(base, '*.su'))
    ci_files = glob.glob(os.path.join(base, '*.ci'))
    for su in su_files:
        if not os.path.isfile(su):
            continue                          # 산출물 디렉토리에 없는 파일은 건너뜀 (방어)
        frames.update(parse_su(su))
    for ci in ci_files:
        if not os.path.isfile(ci):
            continue
        for s, ts in parse_ci(ci).items():
            edges[s].update(ts)

    print('=' * 62)
    print('태스크별 최악 스택 사용량 (정적 분석, Cortex-M4 -O2)')
    print('=' * 62)
    total = 0
    for r in roots:
        memo = {}
        val, why = worst_stack(r, frames, edges, memo, set())
        print(f'{r:<28} {val:>5} bytes (우리 코드 프레임 하한)')
        total = max(total, val)
    print('-' * 62)
    print(f'태스크 최대 스택(하한, 외부 프레임 제외) {total:>5} bytes')
    print('  * FreeRTOS/HAL/libgcc 등 외부 함수 프레임은 별도로 더해져야 함')

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print('사용: python3 stack_analyzer.py vTask_Watchdog vTask_CAN_Rx ...')
        sys.exit(1)
    main()
