#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
rta_analysis.py — 응답시간 분석(Response Time Analysis) 계산기
------------------------------------------------------------------
main_fixed.c 의 태스크셋에 대해 Liu & Layland 이용률 검사와
Audsley 계열의 고정점 반복 응답시간 분석(RTA)을 수행합니다.

수식:  R_i^{k+1} = C_i + B_i + Σ_{j∈hp(i)} ⌈ R_i^k / T_j ⌉ · C_j
종료:  R_i^{k+1} == R_i^k  (수렴)
판정:  R_i ≤ D_i 이면 스케줄가능

가정: 모든 태스크의 우선순위는 서로 다르다(고정 우선순위 선점형 표준 모델).
FreeRTOS는 동일 우선순위를 타임슬라이싱하므로, 동순위가 존재하면 본 RTA는
그 간섭을 반영하지 못한다(결과가 낙관 편향). → 2026-08-18 2차 수정으로
Sensor=4 > Actuator=3 으로 분리하여 이 가정을 코드가 만족하도록 함.
"""
import math

# ---------------------------------------------------------------------------
# 태스크셋 정의 (T: 주기/최소도착간격 ms, C: WCET 추정 ms, p: 우선순위(클수록 높음), D: 데드라인 ms)
#   WCET 는 "추정치"이며, 실측(DWT/SystemView) 후 반드시 재계산해야 함.
# ---------------------------------------------------------------------------
TASKS = [
    # name        T        C       p    D          B(차단시간)
    ("Watchdog", 100.0,   0.05,   6, 1000.0,     0.00),  # D=IWDG 타임아웃(1000ms) > T
    ("CAN_Rx",     1.0,   0.04,   5,    1.0,     0.05),  # 산발(sporadic), 최소도착 1ms 가정; B=액추에이터 뮤텍스 최악 임계구역(PI로 유계)
    ("Sensor",    10.0,   0.80,   4,   10.0,     0.00),  # I2C 블로킹 읽기 지배; (2차 수정) Actuator와 우선순위 분리(RMS 정합)
    ("Actuator",  20.0,   0.05,   3,   20.0,     0.01),  # 큐 대기 20ms, 뮤텍스 차단 10us
    ("Debug",     50.0,   6.00,   2,   50.0,     0.00),  # printf 블로킹(115200) 지배; 센서 큐는 keep-latest(길이 1)로 소비 지연 무관
    ("Firmware", 5000.0,  0.10,   1, 5000.0,     0.00),
]

def rta(task, tasks):
    """단일 태스크의 최악 응답시간(WCRT)을 고정점 반복으로 계산"""
    name, T, C, p, D, B = task
    hp = [t for t in tasks if t[3] > p]          # 더 높은 우선순위 태스크 집합 (인덱스 3=우선순위)
    R = C                                         # R^0 = C
    iters = 0
    while iters < 1000:
        s = C + B
        for (_n, _T, _C, _p, _D, _B) in hp:
            s += math.ceil(R / _T) * _C
        iters += 1
        if abs(s - R) < 1e-9:
            return R, iters
        if s > D:                                 # 데드라인 초과 → 비스케줄가능
            return s, iters
        R = s
    return R, iters

def run_scenario(tasks, label):
    print("=" * 78)
    print(f"[시나리오] {label}")
    print("=" * 78)
    # 이용률 계산
    U_total = sum(t[2] / t[1] for t in tasks)   # U_i = C_i / T_i (인덱스: 1=T, 2=C)
    n = len(tasks)
    prios = [t[3] for t in tasks]
    if len(set(prios)) != len(prios):
        print("  ⚠️ 경고: 동일 우선순위 태스크 존재 → FreeRTOS 타임슬라이싱 간섭이")
        print("     본 RTA에 모델링되지 않음(결과는 낙관 편향). 우선순위 분리 권장.")
    ll_bound = n * (2 ** (1 / n) - 1)
    prod = 1.0
    for t in tasks:
        prod *= (t[2] / t[1] + 1)
    print(f"  태스크 수 n              = {n}")
    print(f"  전체 이용률 U            = {U_total:.4f} ({U_total*100:.2f}%)")
    print(f"  Liu-Layland 상한 n(2^{1/n}-1) = {ll_bound:.4f} ({ll_bound*100:.2f}%)")
    print(f"  Bini 쌍곡선 상한 ∏(U+1) = {prod:.4f}  (≤ 2 이면 통과: {'PASS' if prod <= 2.0 else 'FAIL'})")
    print(f"  이용률 검사              : {'PASS' if U_total <= ll_bound else 'LL한계 초과 → RTA로 판정'}")
    print()
    print(f"  {'태스크':<10}{'T(ms)':>8}{'C(ms)':>8}{'우선순위':>8}{'D(ms)':>8}{'R(ms)':>10}{'반복':>5}  판정")
    print("  " + "-" * 70)
    all_ok = True
    for t in tasks:
        name, T, C, p, D, B = t
        R, it = rta(t, tasks)
        ok = R <= D
        all_ok = all_ok and ok
        print(f"  {name:<10}{T:>8.2f}{C:>8.2f}{p:>8}{D:>8.2f}{R:>10.4f}{it:>5}  {'✅' if ok else '❌'}")
    print()
    print(f"  종합 판정: {'전 태스크 스케줄가능 ✅' if all_ok else '스케줄 불가능 ❌'}")
    print()
    return U_total

# ---------------------------------------------------------------------------
# 시나리오 A: 현재 코드 그대로 (기준)
# ---------------------------------------------------------------------------
run_scenario(TASKS, "A: 현재 코드 (Watchdog=6, CAN=5, Sensor=4, Actuator=3, Debug=2, Firmware=1)")

# ---------------------------------------------------------------------------
# 시나리오 B: 엄격 RMS — 전 태스크를 주기 오름차순(우선순위 내림차순)으로 재배정
#   CAN(1ms)=6 > Sensor(10ms)=5 > Actuator(20ms)=4 > Debug(50ms)=3
#   > Watchdog(100ms)=2 > Firmware(5s)=1
# ---------------------------------------------------------------------------
tasks_b = [list(t) for t in TASKS]
rms_prio = {"CAN_Rx": 6, "Sensor": 5, "Actuator": 4, "Debug": 3, "Watchdog": 2, "Firmware": 1}
for t in tasks_b:
    t[3] = rms_prio[t[0]]  # 인덱스 3 = 우선순위
run_scenario([tuple(t) for t in tasks_b], "B: 엄격 RMS (CAN=6, Sensor=5, Actuator=4, Debug=3, Watchdog=2, Firmware=1)")

# ---------------------------------------------------------------------------
# 시나리오 C: printf 비활성화 (Debug C=6ms → 0.1ms, 인터럽트/DMA 전송 가정)
# ---------------------------------------------------------------------------
tasks_c = [list(t) for t in TASKS]
for t in tasks_c:
    if t[0] == "Debug":
        t[2] = 0.1   # 인덱스 2 = C (실행시간)
run_scenario([tuple(t) for t in tasks_c], "C: printf 비활성화 (Debug C=6ms → 0.1ms)")
