#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
build_server.py — 학습 프로그램용 정적 서버 + 빌드 API
  GET  /                 → 01_learning_program/ 정적 파일 서빙 (learning_program.html 등)
  POST /api/build         → 사용자 코드를 gcc -fsyntax-only 로 컴파일 체크
  POST /api/build-project → 전체 프로젝트(main_build.c + bsp/*.c) 컴파일 체크

gcc 는 스텁 헤더(stubs/)로 STM32 HAL/FreeRTOS 타입을 대체하여 문법·타입 검사만 수행한다.
호스트 gcc(x86)로 문법·타입만 보는 것이므로 실행 파일이 나오지는 않는다.
진짜 크로스 빌드는 02_source_code/Makefile 이 한다.

경로는 전부 이 스크립트 위치에서 계산한다. 예전에는 작성 당시의 절대 경로
(/home/user/commented 등)가 박혀 있어서 저장소를 다른 곳에 두면 동작하지 않았다.

사용법:
    python3 build_server.py [포트]
"""
import http.server
import socketserver
import subprocess
import json
import os
import shutil
import sys
from glob import glob

HERE = os.path.dirname(os.path.abspath(__file__))          # .../01_learning_program/server
WEBROOT = os.path.dirname(HERE)                            # .../01_learning_program
REPO = os.path.dirname(WEBROOT)                            # .../stm32-learning
SRC = os.path.join(REPO, '02_source_code')

STUBS = os.path.join(HERE, 'stubs')
BSP = os.path.join(SRC, 'bsp')
BUILD_MAIN = os.path.join(HERE, 'commented_build', 'main_build.c')
MAKE_STUBS = os.path.join(HERE, 'make_stubs.py')
MAKE_BUILD_MAIN = os.path.join(HERE, 'make_build_main.py')

# -Wno-int-to-pointer-cast: 타깃은 32비트라 (uint32_t)주소 → 포인터 캐스트가
# 정상이지만, 64비트 호스트 gcc 로 검사하면 폭이 안 맞는다고 경고한다.
# 타깃에는 없는 경고라 여기서만 끈다.
BASE_CMD = ['gcc', '-fsyntax-only', '-std=gnu11', '-Wall',
            '-Wno-int-to-pointer-cast',
            '-include', os.path.join(STUBS, 'extra_defs.h'),
            '-I', STUBS, '-I', BSP]


def run_compile(cmd):
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=20)
        return {'ok': r.returncode == 0, 'output': (r.stderr or '').strip()}
    except subprocess.TimeoutExpired:
        return {'ok': False, 'output': '컴파일 시간 초과(20초)'}
    except Exception as e:  # noqa: BLE001
        return {'ok': False, 'output': '빌드 서버 오류: %s' % e}


def build_user_code(code):
    """사용자 코드에 스텁 헤더를 자동 포함하여 컴파일 체크.
    FreeRTOS/HAL/CAN 타입을 미리 선언하므로 include 없이 함수 단위도 검사 가능하다."""
    path = '/tmp/user_check.c'
    prologue = (
        '#include <stdint.h>\n'
        '#include <stddef.h>\n'
        '#include "FreeRTOS.h"\n'
        '#include "task.h"\n'
        '#include "queue.h"\n'
        '#include "semphr.h"\n'
        '#include "stm32f4xx_hal.h"\n'
        '#include "bsp_can.h"\n'
    )
    with open(path, 'w', encoding='utf-8') as fp:
        fp.write(prologue)
        fp.write(code)
    return run_compile(BASE_CMD + [path])


def build_project():
    """전체 프로젝트 컴파일 체크. 소스 변경 반영을 위해 스텁/빌드메인 재생성.

    bsp/*.c 중 일부는 호스트 스텁만으로는 검사할 수 없어 제외한다.
    - fw_image.c  : CMSIS 헤더와 실제 레지스터 정의가 필요하다.
    - 부트로더 계층: 스텁에 없는 플래시 레지스터를 직접 건드린다.
    이 파일들은 02_source_code 에서 arm-none-eabi-gcc 로 실제 빌드되며 검증된다.
    """
    subprocess.run([sys.executable, MAKE_STUBS], capture_output=True, timeout=20)
    subprocess.run([sys.executable, MAKE_BUILD_MAIN], capture_output=True, timeout=20)

    skip = {'fw_image.c'}
    sources = [f for f in sorted(glob(os.path.join(BSP, '*.c')))
               if os.path.basename(f) not in skip]

    missing = [p for p in [BUILD_MAIN] if not os.path.isfile(p)]
    if missing:
        return {'ok': False,
                'output': '생성되지 않은 파일: %s\n'
                          'make_build_main.py 를 직접 실행해 원인을 확인하세요.'
                          % ', '.join(missing)}

    return run_compile(BASE_CMD + [BUILD_MAIN] + sources)


def run_cppcheck(cmd):
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=20)
        out = (r.stdout or '').strip() + '\n' + (r.stderr or '').strip()
        return {'available': True, 'ok': True, 'output': out.strip()}
    except subprocess.TimeoutExpired:
        return {'available': True, 'ok': False, 'output': 'cppcheck 시간 초과(20초)'}
    except Exception as e:  # noqa: BLE001
        return {'available': True, 'ok': False, 'output': 'cppcheck 오류: %s' % e}


def cppcheck_code(code):
    """cppcheck 정적 분석. (MISRA addon 은 라이선스 문제로 제외)"""
    if not shutil.which('cppcheck'):
        return {'available': False,
                'note': 'cppcheck 가 설치되어 있지 않습니다. (서버 세션에서 sudo apt-get install cppcheck 필요)'}
    path = '/tmp/user_check.c'
    with open(path, 'w', encoding='utf-8') as fp:
        fp.write(code)
    cmd = ['cppcheck', '--std=c11',
           '--enable=warning,style,performance,portability',
           '--inconclusive', '--force', '--quiet',
           '--suppress=missingIncludeSystem', '--suppress=missingInclude',
           '--template={file}:{line}: [{severity}] {message}', path]
    return run_cppcheck(cmd)


class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=WEBROOT, **kwargs)

    def do_POST(self):
        if self.path == '/api/build':
            length = int(self.headers.get('Content-Length', 0))
            data = json.loads(self.rfile.read(length) or b'{}')
            result = build_user_code(data.get('code', ''))
            self._reply(result)
        elif self.path == '/api/build-project':
            self._reply(build_project())
        elif self.path == '/api/cppcheck':
            length = int(self.headers.get('Content-Length', 0))
            data = json.loads(self.rfile.read(length) or b'{}')
            self._reply(cppcheck_code(data.get('code', '')))
        else:
            self.send_error(404)

    def _reply(self, obj):
        body = json.dumps(obj, ensure_ascii=False).encode('utf-8')
        self.send_response(200)
        self.send_header('Content-Type', 'application/json; charset=utf-8')
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def log_message(self, fmt, *args):
        pass  # 로그 출력 억제


if __name__ == '__main__':
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8080
    if not shutil.which('gcc'):
        print('경고: gcc 가 없습니다. 빌드 API 는 실패하지만 서버는 뜹니다.')
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(('0.0.0.0', port), Handler) as httpd:
        print('build_server listening on 0.0.0.0:%d' % port)
        print('  웹루트   : %s' % WEBROOT)
        print('  BSP 소스 : %s' % BSP)
        print('  → http://localhost:%d/learning_program.html' % port)
        httpd.serve_forever()
