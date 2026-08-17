#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
build_server.py — 학습 프로그램용 정적 서버 + 빌드 API
  GET  /                 → /home/user 정적 파일 서빙 (learning_program.html 등)
  POST /api/build         → 사용자 코드를 gcc -fsyntax-only 로 컴파일 체크
  POST /api/build-project → 전체 프로젝트(main_build.c + bsp/*.c) 컴파일 체크

gcc 는 스텁 헤더(stubs/)로 STM32 HAL/FreeRTOS 타입을 대체하여 문법·타입 검사만 수행한다.
"""
import http.server
import socketserver
import subprocess
import json
import os
import shutil
from glob import glob

ROOT = '/home/user'
STUBS = os.path.join(ROOT, 'stubs')
BSP = os.path.join(ROOT, 'commented', 'bsp')
BUILD_MAIN = os.path.join(ROOT, 'commented_build', 'main_build.c')

BASE_CMD = ['gcc', '-fsyntax-only', '-std=gnu11', '-Wall',
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
    """전체 프로젝트 컴파일 체크. 소스 변경 반영을 위해 스텁/빌드메인 재생성."""
    subprocess.run(['python3', os.path.join(ROOT, 'make_stubs.py')],
                   capture_output=True, timeout=20)
    subprocess.run(['python3', os.path.join(ROOT, 'make_build_main.py')],
                   capture_output=True, timeout=20)
    files = [BUILD_MAIN] + sorted(glob(os.path.join(BSP, '*.c')))
    return run_compile(BASE_CMD + files)


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
        super().__init__(*args, directory=ROOT, **kwargs)

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
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(('0.0.0.0', 8080), Handler) as httpd:
        print('build_server listening on 0.0.0.0:8080')
        httpd.serve_forever()
