#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""make_image.py — 앱 바이너리에 갱신용 32바이트 헤더를 붙입니다.

빌드 산출물 app.bin 은 그냥 기계어 덩어리라서, 받는 쪽에서 "이게 정말 우리
펌웨어인지 / 전송 중 깨지지 않았는지"를 알 방법이 없습니다. 그래서 앞에
헤더를 붙입니다. 이 헤더는 bsp/fw_image.h 의 FwImageHeader_t 와 바이트 단위로
같은 구조이며, CRC-32 알고리즘도 bsp/fw_image.c 와 동일합니다.

    +0x00  ulMagic         0x4D465733
    +0x04  ulHeaderVersion 0x00010000
    +0x08  ulImageSize     본문 크기 (4의 배수로 패딩됨)
    +0x0C  ulImageCrc32    본문 전체의 CRC-32
    +0x10  ulFwVersion     펌웨어 버전 (예: 0x00010203 = v1.2.3)
    +0x14  ulLoadAddr      실행 주소 (0x08004000)
    +0x18  ulReserved      0
    +0x1C  ulHeaderCrc32   앞 28바이트의 CRC-32
    +0x20  본문 시작

사용:
    python3 make_image.py --input app.bin --output app_image.bin
    python3 make_image.py --input app.bin --output app_image.bin --version 1.2.3
"""

import argparse
import struct
import sys

# bsp/fw_image.h 와 반드시 일치해야 하는 상수들입니다.
FW_IMAGE_MAGIC = 0x4D465733           # 이미지 헤더 매직 값입니다.
FW_IMAGE_HEADER_VERSION = 0x00010000  # 헤더 포맷 버전 1.0 입니다.
FW_IMAGE_HEADER_SIZE = 32             # 헤더 크기(바이트)입니다.

# bsp/flash_map.h 와 반드시 일치해야 하는 상수들입니다.
APP_REGION_ADDR = 0x08004000  # 앱 영역 시작 주소(섹터 1)입니다.
APP_REGION_SIZE = 112 * 1024  # 앱 영역 크기(섹터 1~4)입니다.
RAM_BASE = 0x20000000         # SRAM 시작 주소입니다.
RAM_SIZE = 128 * 1024         # SRAM 크기입니다.

# CRC-32 (IEEE 802.3) — bsp/fw_image.c 의 구현과 동일한 결과를 냅니다.
#   다항식 0xEDB88320 (반전 표현), 초기값 0xFFFFFFFF, 최종 XOR 0xFFFFFFFF
#   zlib.crc32() 와 같은 값이지만, 어떤 알고리즘인지 코드로 남기려고 직접 씁니다.
_CRC32_POLY = 0xEDB88320


def _build_table():
    """CRC-32 조회 테이블 256개 항목을 만듭니다."""
    table = []
    for byte in range(256):
        crc = byte
        for _ in range(8):
            crc = (crc >> 1) ^ (_CRC32_POLY if (crc & 1) else 0)
        table.append(crc)
    return table


_CRC32_TABLE = _build_table()


def crc32(data):
    """바이트열의 CRC-32 를 계산합니다. (fw_image.c 의 FwImage_Crc32 와 동일)"""
    crc = 0xFFFFFFFF
    for byte in data:
        crc = _CRC32_TABLE[(crc ^ byte) & 0xFF] ^ (crc >> 8)
    return crc ^ 0xFFFFFFFF


def parse_version(text):
    """버전 문자열을 32비트 정수로 바꿉니다.

    받는 형식:
        '1.2.3'      → 0x00010203  (major.minor.patch)
        '0x00010203' → 0x00010203  (16진수 직접 지정)
        '65539'      → 0x00010003  (10진수 직접 지정)
    """
    text = text.strip()
    if '.' in text:
        parts = text.split('.')
        if len(parts) != 3:
            raise argparse.ArgumentTypeError(
                "버전은 'major.minor.patch' 형식이어야 합니다: " + text)
        try:
            major, minor, patch = (int(p) for p in parts)
        except ValueError:
            raise argparse.ArgumentTypeError('버전 각 자리는 정수여야 합니다: ' + text)
        for value, name in ((major, 'major'), (minor, 'minor'), (patch, 'patch')):
            if not 0 <= value <= 0xFF:
                raise argparse.ArgumentTypeError(
                    '%s 는 0~255 범위여야 합니다: %d' % (name, value))
        return (major << 16) | (minor << 8) | patch
    try:
        return int(text, 0) & 0xFFFFFFFF
    except ValueError:
        raise argparse.ArgumentTypeError('버전을 해석할 수 없습니다: ' + text)


def check_vector_table(image, load_addr):
    """벡터 테이블의 앞 두 워드(초기 SP, Reset 핸들러 주소)가 말이 되는지 봅니다.

    이 검사는 bsp/fw_image.c 의 FwImage_IsVectorTableSane() 과 같은 규칙입니다.
    엉뚱한 파일(예: 부트로더 바이너리나 .elf)을 실수로 이미지로 만들었을 때
    장치에 넣기 전에 호스트에서 잡아내는 것이 목적입니다.
    """
    if len(image) < 8:
        return '이미지가 8바이트보다 작아 벡터 테이블이 없습니다.'

    initial_sp, reset_pc = struct.unpack('<II', image[:8])

    if not (RAM_BASE <= initial_sp <= RAM_BASE + RAM_SIZE):
        return ('초기 스택 포인터가 SRAM 범위를 벗어납니다: 0x%08X '
                '(기대 범위 0x%08X~0x%08X)' % (initial_sp, RAM_BASE, RAM_BASE + RAM_SIZE))

    # Thumb 명령이므로 Reset 핸들러 주소의 최하위 비트는 반드시 1이어야 합니다.
    if (reset_pc & 1) == 0:
        return ('Reset 핸들러 주소의 Thumb 비트가 0입니다: 0x%08X' % reset_pc)

    if not (load_addr <= (reset_pc & ~1) < load_addr + APP_REGION_SIZE):
        return ('Reset 핸들러 주소가 앱 영역 밖입니다: 0x%08X '
                '(기대 범위 0x%08X~0x%08X)'
                % (reset_pc, load_addr, load_addr + APP_REGION_SIZE))

    return None


def main():
    parser = argparse.ArgumentParser(
        description='앱 바이너리에 갱신용 32바이트 헤더를 붙입니다.')
    parser.add_argument('--input', '-i', required=True,
                        help='입력 바이너리 (예: build/app/app.bin)')
    parser.add_argument('--output', '-o', required=True,
                        help='출력 이미지 (예: build/app/app_image.bin)')
    parser.add_argument('--version', '-v', type=parse_version, default=0x00010000,
                        help="펌웨어 버전. '1.2.3' 또는 '0x00010203' (기본 0x00010000)")
    parser.add_argument('--load-addr', type=lambda s: int(s, 0), default=APP_REGION_ADDR,
                        help='실행 주소 (기본 0x08004000)')
    parser.add_argument('--skip-vector-check', action='store_true',
                        help='벡터 테이블 정상 여부 검사를 건너뜁니다.')
    args = parser.parse_args()

    try:
        with open(args.input, 'rb') as handle:
            image = handle.read()
    except OSError as exc:
        sys.exit('입력 파일을 읽을 수 없습니다: %s' % exc)

    if not image:
        sys.exit('입력 파일이 비어 있습니다: %s' % args.input)

    # 플래시는 워드(4바이트) 단위로 기록하므로 본문 길이를 4의 배수로 맞춥니다.
    # 남는 자리는 0xFF(소거 상태)로 채워야 나중에 덧쓰기가 가능합니다.
    padding = (-len(image)) % 4
    if padding:
        image += b'\xFF' * padding

    if len(image) > APP_REGION_SIZE:
        sys.exit('이미지가 앱 영역보다 큽니다: %d B > %d B'
                 % (len(image), APP_REGION_SIZE))

    if not args.skip_vector_check:
        problem = check_vector_table(image, args.load_addr)
        if problem is not None:
            sys.exit('벡터 테이블 검사 실패: %s\n'
                     '  (의도한 것이라면 --skip-vector-check 를 쓰십시오)' % problem)

    image_crc = crc32(image)

    # 헤더 앞 28바이트를 먼저 만들고, 그 CRC 를 마지막 4바이트에 넣습니다.
    header_body = struct.pack(
        '<7I',
        FW_IMAGE_MAGIC,           # ulMagic
        FW_IMAGE_HEADER_VERSION,  # ulHeaderVersion
        len(image),               # ulImageSize
        image_crc,                # ulImageCrc32
        args.version,             # ulFwVersion
        args.load_addr,           # ulLoadAddr
        0,                        # ulReserved
    )
    header = header_body + struct.pack('<I', crc32(header_body))
    assert len(header) == FW_IMAGE_HEADER_SIZE

    try:
        with open(args.output, 'wb') as handle:
            handle.write(header)
            handle.write(image)
    except OSError as exc:
        sys.exit('출력 파일을 쓸 수 없습니다: %s' % exc)

    print('갱신 이미지 생성: %s' % args.output)
    print('  본문 크기   : %d B (%d B 패딩)' % (len(image), padding))
    print('  본문 CRC-32 : 0x%08X' % image_crc)
    print('  펌웨어 버전 : 0x%08X (v%d.%d.%d)'
          % (args.version, (args.version >> 16) & 0xFF,
             (args.version >> 8) & 0xFF, args.version & 0xFF))
    print('  실행 주소   : 0x%08X' % args.load_addr)
    print('  전체 크기   : %d B (헤더 32 B 포함)' % (len(header) + len(image)))


if __name__ == '__main__':
    main()
