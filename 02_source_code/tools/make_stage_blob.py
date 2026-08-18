#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
make_stage_blob.py — 스테이징 영역(섹터 5)에 곧바로 구울 수 있는 블롭을 만듭니다.

왜 필요한가
-----------
스테이징 패턴의 정상 경로는 "앱이 CAN 으로 이미지를 받아 스테이징에 기록"이지만,
전송 프로토콜(분할·재전송·흐름 제어)은 아직 구현되지 않았습니다. 그렇다고 부트로더의
검증·설치·복구 동작을 실물에서 못 보는 것은 아닙니다. 전송 계층을 빼고 **디버거로
스테이징 영역에 직접 이미지를 써 넣으면** 그 뒤의 모든 경로가 실제와 동일하게 돕니다.

이 스크립트는 make_image.py 가 만든 이미지를 스테이징 영역의 실제 배치에 맞춰
재배열합니다.

    +0x000  32B 이미지 헤더        (make_image.py 산출물의 앞 32B)
    +0x020  0xFF 채움
    +0x100  설치 완료 표식 워드    → 0xFFFFFFFF (소거 상태여야 함)
    +0x104  0xFF 채움
    +0x200  이미지 본문            (make_image.py 산출물의 32B 이후)

+0x100 이 소거 상태(0xFFFFFFFF)인 것이 중요합니다. 여기에 'INST'(0x494E5354)가
찍혀 있으면 부트로더와 앱 모두 "이미 설치된 이미지"로 보고 설치를 건너뜁니다.

사용법
------
    # 정상 이미지
    python3 tools/make_stage_blob.py -i build/app/app_image.bin -o build/stage.bin
    st-flash --reset write build/stage.bin 0x08020000

    # 본문이 손상된 이미지 (CRC 거부 경로 시험용)
    python3 tools/make_stage_blob.py -i build/app/app_image.bin -o build/stage_bad.bin --corrupt

    # 헤더 매직이 깨진 이미지 (헤더 거부 경로 시험용)
    python3 tools/make_stage_blob.py -i build/app/app_image.bin -o build/stage_nomagic.bin --break-magic
"""
import argparse
import struct
import sys

# flash_map.h 와 반드시 일치해야 하는 값들입니다.
STAGE_REGION_ADDR = 0x08020000  # 스테이징 영역 시작 주소 (섹터 5)
STAGE_REGION_SIZE = 128 * 1024  # 스테이징 영역 크기
STAGE_META_SIZE = 0x200         # 메타데이터 영역 크기 (헤더 + 설치 표식)
STAGE_INSTALLED_OFF = 0x100     # 설치 완료 표식 워드의 오프셋
FW_IMAGE_MAGIC = 0x4D465733     # 이미지 헤더 매직 ('3WFM' 리틀엔디언 표기, fw_image.h 와 동일)
FW_IMAGE_HEADER_SIZE = 32       # 이미지 헤더 크기


def main():
    parser = argparse.ArgumentParser(
        description='갱신 이미지를 스테이징 영역 배치로 재구성합니다.')
    parser.add_argument('--input', '-i', required=True,
                        help='make_image.py 가 만든 이미지 (예: build/app/app_image.bin)')
    parser.add_argument('--output', '-o', required=True,
                        help='출력 블롭 (0x08020000 에 기록할 파일)')
    parser.add_argument('--corrupt', action='store_true',
                        help='본문 1바이트를 뒤집어 CRC 검증이 실패하도록 만듭니다.')
    parser.add_argument('--corrupt-offset', type=lambda s: int(s, 0), default=0x40,
                        help='손상시킬 본문 내 오프셋 (기본 0x40)')
    parser.add_argument('--break-magic', action='store_true',
                        help='헤더 매직을 0 으로 만들어 헤더 검사에서 거부되게 합니다.')
    args = parser.parse_args()

    with open(args.input, 'rb') as handle:
        image = handle.read()

    if len(image) <= FW_IMAGE_HEADER_SIZE:
        print('오류: 입력 파일이 헤더보다 작습니다. make_image.py 산출물이 맞습니까?',
              file=sys.stderr)
        return 1

    header = bytearray(image[:FW_IMAGE_HEADER_SIZE])
    body = bytearray(image[FW_IMAGE_HEADER_SIZE:])

    magic, _hdrver, size, imgcrc, fwver, load, _rsv, hdrcrc = struct.unpack('<8I', header)
    if magic != FW_IMAGE_MAGIC:
        print('오류: 매직이 0x%08X 입니다. 0x%08X 여야 합니다. '
              'make_image.py 를 거치지 않은 파일 같습니다.' % (magic, FW_IMAGE_MAGIC),
              file=sys.stderr)
        return 1

    if size != len(body):
        print('경고: 헤더의 크기(%d)와 실제 본문(%d)이 다릅니다.' % (size, len(body)),
              file=sys.stderr)

    # --- 고장 주입 ---------------------------------------------------------
    injected = '없음'
    if args.corrupt:
        if args.corrupt_offset >= len(body):
            print('오류: 손상 오프셋이 본문 크기를 벗어납니다.', file=sys.stderr)
            return 1
        before = body[args.corrupt_offset]
        body[args.corrupt_offset] = before ^ 0xFF
        injected = ('본문 +0x%X: 0x%02X → 0x%02X (CRC 불일치 유도)'
                    % (args.corrupt_offset, before, body[args.corrupt_offset]))
    if args.break_magic:
        header[0:4] = struct.pack('<I', 0)
        injected = '헤더 매직 → 0x00000000 (헤더 검사 거부 유도)'

    # --- 스테이징 배치로 재구성 -------------------------------------------
    blob = bytearray(b'\xFF' * STAGE_META_SIZE)
    blob[0:FW_IMAGE_HEADER_SIZE] = header
    # +0x100 의 설치 표식은 소거 상태(0xFFFFFFFF)로 남겨 둡니다. 위의 0xFF 채움이 그 역할입니다.
    blob += body

    if len(blob) > STAGE_REGION_SIZE:
        print('오류: 블롭(%d B)이 스테이징 영역(%d B)보다 큽니다.'
              % (len(blob), STAGE_REGION_SIZE), file=sys.stderr)
        return 1

    with open(args.output, 'wb') as handle:
        handle.write(blob)

    mark = struct.unpack('<I', bytes(blob[STAGE_INSTALLED_OFF:STAGE_INSTALLED_OFF + 4]))[0]

    print('스테이징 블롭 생성: %s' % args.output)
    print('  기록 주소   : 0x%08X (섹터 5)' % STAGE_REGION_ADDR)
    print('  블롭 크기   : %d B (메타 %d B + 본문 %d B)'
          % (len(blob), STAGE_META_SIZE, len(body)))
    print('  헤더 매직   : 0x%08X' % struct.unpack('<I', bytes(header[0:4]))[0])
    print('  본문 CRC32  : 0x%08X (헤더 기록값)' % imgcrc)
    print('  헤더 CRC32  : 0x%08X' % hdrcrc)
    print('  펌웨어 버전 : 0x%08X' % fwver)
    print('  실행 주소   : 0x%08X' % load)
    print('  설치 표식   : 0x%08X %s'
          % (mark, '(소거 상태 — 정상)' if mark == 0xFFFFFFFF else '(!! 소거 상태가 아님)'))
    print('  고장 주입   : %s' % injected)
    print()
    print('  기록 명령   : st-flash --reset write %s 0x%08X' % (args.output, STAGE_REGION_ADDR))
    return 0


if __name__ == '__main__':
    sys.exit(main())
