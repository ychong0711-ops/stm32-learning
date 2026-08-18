# server/ — 학습 프로그램 도구

`learning_program.html` 은 단일 파일 오프라인 앱이다. 그냥 브라우저로 열어도
대부분 동작하고, 여기 있는 서버를 띄우면 **실제 gcc 컴파일 검사**까지 된다.

---

## 지금 쓰는 것

| 파일 | 역할 |
|---|---|
| `build_server.py` | 정적 서버 + 빌드 API. 학습 프로그램을 제대로 쓰려면 이걸 띄운다. |
| `refresh_html_sources.py` | HTML 안에 박힌 소스 스냅샷을 저장소 기준으로 다시 찍는다. |
| `make_stubs.py` | 호스트 gcc 검사용 스텁 헤더 생성 → `stubs/` |
| `make_build_main.py` | `main_fixed.c` → 검사용 단일 파일 → `commented_build/main_build.c` |

### 띄우기

```bash
cd 01_learning_program/server
python3 build_server.py            # http://localhost:8080/learning_program.html
python3 build_server.py 9000       # 포트 지정
```

`cppcheck` 가 있으면 정적 분석 탭도 켜진다. 없으면 그 탭만 비활성화되고
나머지는 정상 동작한다. (`sudo apt-get install -y cppcheck`)

### 소스를 고친 뒤

`02_source_code/` 를 수정하면 HTML 안의 스냅샷이 옛날 것으로 남는다.

```bash
python3 refresh_html_sources.py            # 갱신
python3 refresh_html_sources.py --check    # 갱신 필요 여부만 (다르면 종료코드 1)
```

---

## 빌드 API 가 검사하는 것과 못 하는 것

`gcc -fsyntax-only` 를 **호스트(x86-64)** 에서 돌린다. 문법과 타입만 본다.
실행 파일은 나오지 않는다. **진짜 크로스 빌드는 `02_source_code/Makefile`** 이
`arm-none-eabi-gcc` 로 한다.

STM32 HAL 과 FreeRTOS 실물 헤더 대신 `stubs/` 의 최소 선언을 쓴다. 그래서:

- 함수 이름 오타, 인자 개수/타입 불일치, 선언 없는 호출 → **잡힌다**
- 레지스터 비트 값이 맞는지, 실제로 도는지 → **못 잡는다**

`/api/build-project` 는 `bsp/fw_image.c` 를 건너뛴다. CMSIS 헤더와 실제 레지스터
정의가 필요해서 스텁으로는 검사할 수 없다. 이 파일은 크로스 빌드에서 검증된다.

`-Wno-int-to-pointer-cast` 를 켜 둔다. 타깃은 32비트라 `(uint32_t)`주소를
포인터로 캐스트하는 게 정상인데, 64비트 호스트에서 검사하면 폭이 안 맞는다고
경고한다. 타깃에 존재하지 않는 경고라 껐다.

### 스텁을 고쳐야 할 때

새 HAL 함수나 매크로를 쓰기 시작하면 `/api/build-project` 가 "undeclared" 로
실패한다. `make_stubs.py` 에 선언을 추가하고 다시 돌리면 된다.

```bash
python3 make_stubs.py
```

`stubs/` 는 **생성물이지만 저장소에 커밋되어 있다.** 서버를 처음 띄우는 사람이
생성 단계를 몰라도 되게 하기 위해서다. `make_stubs.py` 는 멱등이라 다시 돌려도
같은 내용이 나온다.

---

## 과거 스크립트 (실행하지 말 것)

`build_learning.py`, `build_learning3.py` ~ `build_learning7.py` 는 이 HTML 을
단계적으로 만들어 온 **일회성 UI 주입 스크립트**다. 결과물은 이미
`learning_program.html` 에 반영되어 있다.

이들은 작성 당시의 절대 경로(`/home/user/commented`, `/home/user/learning_program.html`)를
그대로 들고 있어서 **지금 실행하면 실패한다.** 고치지 않고 남겨 둔 이유는,
각 탭이 어떤 의도로 추가됐는지가 docstring 에 기록되어 있어 이력으로서 값이
있고, 되살려서 재실행할 일이 없기 때문이다. (되돌리려면 git 이력을 쓰면 된다)

**소스 스냅샷만 갱신하려는 것이라면 `refresh_html_sources.py` 를 쓴다.**
이 스크립트들이 아니다.
