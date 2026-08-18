#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""make_combined.py — 부트로더와 앱을 하나의 공장 출하용 바이너리로 합칩니다.

부트로더는 0x08000000, 앱은 0x08004000 에 놓입니다. 두 파일을 따로 굽는 대신
합본 하나를 0x08000000 에 굽고 싶을 때 씁니다. (생산 라인, 초기 프로비저닝)

부트로더 영역(16KB)과 앱 사이의 빈 공간은 0xFF 로 채웁니다. 플래시의 소거
상태가 0xFF 이므로, 이렇게 해야 "쓰지 않은 것"과 동일한 결과가 됩니다.

사용:
    python3 make_combined.py --boot boot.bin --app app.bin --output combined.bin
"""

import argparse
import sys

# bsp/flash_map.h 와 반드시 일치해야 하는 상수들입니다.
BOOT_REGION_ADDR = 0x08000000  # 부트로더 영역 시작 주소(섹터 0)입니다.
BOOT_REGION_SIZE = 16 * 1024   # 부트로더 영역 크기입니다.
APP_REGION_ADDR = 0x08004000   # 앱 영역 시작 주소(섹터 1)입니다.
APP_REGION_SIZE = 112 * 1024   # 앱 영역 크기(섹터 1~4)입니다.

ERASED_BYTE = 0xFF  # 플래시 소거 상태의 바이트 값입니다.


def main():
    parser = argparse.ArgumentParser(
        description='부트로더와 앱을 하나의 출하용 바이너리로 합칩니다.')
    parser.add_argument('--boot', required=True, help='부트로더 바이너리 (boot.bin)')
    parser.add_argument('--app', required=True, help='애플리케이션 바이너리 (app.bin)')
    parser.add_argument('--output', '-o', required=True, help='출력 합본 바이너리')
    args = parser.parse_args()

    try:
        with open(args.boot, 'rb') as handle:
            boot = handle.read()
        with open(args.app, 'rb') as handle:
            app = handle.read()
    except OSError as exc:
        sys.exit('입력 파일을 읽을 수 없습니다: %s' % exc)

    if len(boot) > BOOT_REGION_SIZE:
        sys.exit('부트로더가 섹터 0을 넘칩니다: %d B > %d B'
                 % (len(boot), BOOT_REGION_SIZE))

    if len(app) > APP_REGION_SIZE:
        sys.exit('앱이 배정 영역을 넘칩니다: %d B > %d B'
                 % (len(app), APP_REGION_SIZE))

    gap = BOOT_REGION_SIZE - len(boot)  # 부트로더 뒤의 빈 공간 크기입니다.
    combined = boot + bytes([ERASED_BYTE]) * gap + app

    try:
        with open(args.output, 'wb') as handle:
            handle.write(combined)
    except OSError as exc:
        sys.exit('출력 파일을 쓸 수 없습니다: %s' % exc)

    print('합본 이미지 생성: %s' % args.output)
    print('  0x%08X  부트로더 : %6d B (섹터 0에서 %d B 여유)'
          % (BOOT_REGION_ADDR, len(boot), gap))
    print('  0x%08X  앱       : %6d B (%d B 여유)'
          % (APP_REGION_ADDR, len(app), APP_REGION_SIZE - len(app)))
    print('  전체 크기 : %d B' % len(combined))
    print('  기록 예시 : st-flash --reset write %s 0x%08X'
          % (args.output, BOOT_REGION_ADDR))


if __name__ == '__main__':
    main()
