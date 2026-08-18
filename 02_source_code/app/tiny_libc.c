// tiny_libc.c — 표준 C 라이브러리 최소 대체 구현입니다.
//
// [왜 필요한가]
//   이 프로젝트는 -nostdlib 로 링크합니다. (컴파일 검증에 사용한 툴체인에 newlib
//   libc.a 가 없고, 무엇보다 어떤 코드가 링크되는지 전부 눈에 보이는 편이
//   정적 스택 분석/WCET 증빙에 유리하기 때문입니다.)
//   그런데 FreeRTOS 커널과 컴파일러가 생성하는 코드는 아래 함수들을 필요로 합니다.
//     memcpy  : queue.c 의 항목 복사 (xQueueSend/xQueueReceive)
//     memset  : tasks.c 의 TCB 초기화, heap_4.c 의 pvPortCalloc, 스택 패턴 채우기
//     strlen  : tasks.c 의 태스크 이름 길이 검사(configASSERT)
//     strcpy  : tasks.c 의 태스크 상태 문자열 생성
//   구조체 대입이나 큰 배열 초기화에서 컴파일러가 암묵적으로 memcpy/memset 을
//   호출하기도 하므로, 이들은 어떤 경우에도 반드시 존재해야 합니다.
//
// [주의: 자기 재귀 함정]
//   GCC 는 "바이트 단위 복사 루프"를 발견하면 그것을 memcpy() 호출로 바꿔버립니다.
//   memcpy() 안에서 그 최적화가 일어나면 자기 자신을 무한 호출합니다.
//   그래서 아래 함수들에 no-tree-loop-distribute-patterns 최적화 속성을 붙였습니다.
//   (Makefile 에도 같은 플래그를 전역으로 넣어 두었습니다 — 이중 안전장치)
#include <stddef.h>  // size_t 와 NULL 을 사용하기 위해 포함합니다.
#include <stdint.h>  // 고정 폭 정수 타입을 사용하기 위해 포함합니다.

// 루프 → memcpy/memset 자동 치환을 막는 속성입니다. (자기 재귀 방지)
#define TINY_LIBC_NO_PATTERN  __attribute__((optimize("no-tree-loop-distribute-patterns")))

// --------------------------------------------------------------------------
// 메모리 조작
// --------------------------------------------------------------------------

TINY_LIBC_NO_PATTERN
void *memcpy(void *pvDest, const void *pvSrc, size_t xLength)  // 메모리 복사 함수를 정의합니다.
{  // memcpy 본문을 시작합니다.
    uint8_t *pucDest = (uint8_t *)pvDest;  // 목적지를 바이트 포인터로 변환합니다.
    const uint8_t *pucSrc = (const uint8_t *)pvSrc;  // 원본을 바이트 포인터로 변환합니다.

    // 4바이트 정렬이 맞으면 워드 단위로 복사해 속도를 높입니다.
    if ((((uintptr_t)pucDest | (uintptr_t)pucSrc) & 3U) == 0U)  // 양쪽이 모두 4바이트 정렬인지 확인합니다.
    {  // 워드 단위 복사 블록을 시작합니다.
        uint32_t *pulDest = (uint32_t *)pvDest;  // 목적지를 워드 포인터로 변환합니다.
        const uint32_t *pulSrc = (const uint32_t *)pvSrc;  // 원본을 워드 포인터로 변환합니다.
        while (xLength >= 4U)  // 남은 길이가 4바이트 이상인 동안 반복합니다.
        {  // 워드 복사 반복문 본문을 시작합니다.
            *pulDest++ = *pulSrc++;  // 워드 하나를 복사합니다.
            xLength -= 4U;  // 남은 길이를 4 감소시킵니다.
        }  // 워드 복사 반복문을 종료합니다.
        pucDest = (uint8_t *)pulDest;  // 남은 바이트 처리를 위해 바이트 포인터를 갱신합니다.
        pucSrc = (const uint8_t *)pulSrc;  // 남은 바이트 처리를 위해 원본 포인터를 갱신합니다.
    }  // 워드 단위 복사 블록을 종료합니다.

    while (xLength-- > 0U)  // 남은 바이트가 있는 동안 반복합니다.
    {  // 바이트 복사 반복문 본문을 시작합니다.
        *pucDest++ = *pucSrc++;  // 바이트 하나를 복사합니다.
    }  // 바이트 복사 반복문을 종료합니다.

    return pvDest;  // 목적지 포인터를 반환합니다.
}  // memcpy 를 종료합니다.

TINY_LIBC_NO_PATTERN
void *memset(void *pvDest, int iValue, size_t xLength)  // 메모리 채우기 함수를 정의합니다.
{  // memset 본문을 시작합니다.
    uint8_t *pucDest = (uint8_t *)pvDest;  // 목적지를 바이트 포인터로 변환합니다.
    const uint8_t ucValue = (uint8_t)iValue;  // 채울 값을 바이트로 변환합니다.

    if (((uintptr_t)pucDest & 3U) == 0U)  // 목적지가 4바이트 정렬인지 확인합니다.
    {  // 워드 단위 채우기 블록을 시작합니다.
        uint32_t *pulDest = (uint32_t *)pvDest;  // 목적지를 워드 포인터로 변환합니다.
        uint32_t ulPattern = ((uint32_t)ucValue) * 0x01010101U;  // 같은 바이트를 4개 이어붙인 워드를 만듭니다.
        while (xLength >= 4U)  // 남은 길이가 4바이트 이상인 동안 반복합니다.
        {  // 워드 채우기 반복문 본문을 시작합니다.
            *pulDest++ = ulPattern;  // 워드 하나를 채웁니다.
            xLength -= 4U;  // 남은 길이를 4 감소시킵니다.
        }  // 워드 채우기 반복문을 종료합니다.
        pucDest = (uint8_t *)pulDest;  // 남은 바이트 처리를 위해 바이트 포인터를 갱신합니다.
    }  // 워드 단위 채우기 블록을 종료합니다.

    while (xLength-- > 0U)  // 남은 바이트가 있는 동안 반복합니다.
    {  // 바이트 채우기 반복문 본문을 시작합니다.
        *pucDest++ = ucValue;  // 바이트 하나를 채웁니다.
    }  // 바이트 채우기 반복문을 종료합니다.

    return pvDest;  // 목적지 포인터를 반환합니다.
}  // memset 을 종료합니다.

TINY_LIBC_NO_PATTERN
void *memmove(void *pvDest, const void *pvSrc, size_t xLength)  // 영역이 겹쳐도 안전한 복사 함수를 정의합니다.
{  // memmove 본문을 시작합니다.
    uint8_t *pucDest = (uint8_t *)pvDest;  // 목적지를 바이트 포인터로 변환합니다.
    const uint8_t *pucSrc = (const uint8_t *)pvSrc;  // 원본을 바이트 포인터로 변환합니다.

    if (pucDest == pucSrc)  // 같은 주소면 할 일이 없는지 확인합니다.
    {  // 동일 주소 처리 블록을 시작합니다.
        return pvDest;  // 그대로 반환합니다.
    }  // 동일 주소 처리 블록을 종료합니다.

    if (pucDest < pucSrc)  // 목적지가 앞쪽이면 앞에서부터 복사해도 안전한지 확인합니다.
    {  // 정방향 복사 블록을 시작합니다.
        while (xLength-- > 0U)  // 남은 바이트가 있는 동안 반복합니다.
        {  // 정방향 복사 반복문 본문을 시작합니다.
            *pucDest++ = *pucSrc++;  // 바이트 하나를 복사합니다.
        }  // 정방향 복사 반복문을 종료합니다.
    }  // 정방향 복사 블록을 종료합니다.
    else  // 목적지가 뒤쪽이라 뒤에서부터 복사해야 하는 경우입니다.
    {  // 역방향 복사 블록을 시작합니다.
        pucDest += xLength;  // 목적지 포인터를 끝으로 옮깁니다.
        pucSrc += xLength;  // 원본 포인터를 끝으로 옮깁니다.
        while (xLength-- > 0U)  // 남은 바이트가 있는 동안 반복합니다.
        {  // 역방향 복사 반복문 본문을 시작합니다.
            *--pucDest = *--pucSrc;  // 뒤에서부터 바이트 하나를 복사합니다.
        }  // 역방향 복사 반복문을 종료합니다.
    }  // 역방향 복사 블록을 종료합니다.

    return pvDest;  // 목적지 포인터를 반환합니다.
}  // memmove 를 종료합니다.

TINY_LIBC_NO_PATTERN
int memcmp(const void *pvLeft, const void *pvRight, size_t xLength)  // 메모리 비교 함수를 정의합니다.
{  // memcmp 본문을 시작합니다.
    const uint8_t *pucLeft = (const uint8_t *)pvLeft;  // 왼쪽 버퍼를 바이트 포인터로 변환합니다.
    const uint8_t *pucRight = (const uint8_t *)pvRight;  // 오른쪽 버퍼를 바이트 포인터로 변환합니다.

    while (xLength-- > 0U)  // 남은 바이트가 있는 동안 반복합니다.
    {  // 비교 반복문 본문을 시작합니다.
        if (*pucLeft != *pucRight)  // 두 바이트가 다른지 확인합니다.
        {  // 불일치 처리 블록을 시작합니다.
            return (int)*pucLeft - (int)*pucRight;  // 차이를 부호 있는 정수로 반환합니다.
        }  // 불일치 처리 블록을 종료합니다.
        pucLeft++;   // 왼쪽 포인터를 전진시킵니다.
        pucRight++;  // 오른쪽 포인터를 전진시킵니다.
    }  // 비교 반복문을 종료합니다.

    return 0;  // 모든 바이트가 같으면 0 을 반환합니다.
}  // memcmp 를 종료합니다.

// --------------------------------------------------------------------------
// 문자열 조작 (FreeRTOS tasks.c 가 사용하는 최소 집합)
// --------------------------------------------------------------------------

TINY_LIBC_NO_PATTERN
size_t strlen(const char *pcStr)  // 문자열 길이 함수를 정의합니다.
{  // strlen 본문을 시작합니다.
    const char *pcCursor = pcStr;  // 문자열을 순회할 커서를 준비합니다.

    while (*pcCursor != '\0')  // 널 종단 문자를 만날 때까지 반복합니다.
    {  // 길이 계산 반복문 본문을 시작합니다.
        pcCursor++;  // 커서를 다음 문자로 전진시킵니다.
    }  // 길이 계산 반복문을 종료합니다.

    return (size_t)(pcCursor - pcStr);  // 시작과의 차이를 길이로 반환합니다.
}  // strlen 을 종료합니다.

TINY_LIBC_NO_PATTERN
char *strcpy(char *pcDest, const char *pcSrc)  // 문자열 복사 함수를 정의합니다.
{  // strcpy 본문을 시작합니다.
    char *pcCursor = pcDest;  // 목적지 커서를 준비합니다.

    while ((*pcCursor++ = *pcSrc++) != '\0')  // 널 종단 문자까지 한 글자씩 복사합니다.
    {  // 복사 반복문 본문을 시작합니다.
    }  // 복사 반복문 본문을 종료합니다.

    return pcDest;  // 목적지 포인터를 반환합니다.
}  // strcpy 를 종료합니다.

TINY_LIBC_NO_PATTERN
int strcmp(const char *pcLeft, const char *pcRight)  // 문자열 비교 함수를 정의합니다.
{  // strcmp 본문을 시작합니다.
    while ((*pcLeft != '\0') && (*pcLeft == *pcRight))  // 문자가 같고 끝이 아닌 동안 반복합니다.
    {  // 비교 반복문 본문을 시작합니다.
        pcLeft++;   // 왼쪽 포인터를 전진시킵니다.
        pcRight++;  // 오른쪽 포인터를 전진시킵니다.
    }  // 비교 반복문을 종료합니다.

    return (int)(unsigned char)*pcLeft - (int)(unsigned char)*pcRight;  // 첫 불일치 문자의 차이를 반환합니다.
}  // strcmp 를 종료합니다.

TINY_LIBC_NO_PATTERN
int strncmp(const char *pcLeft, const char *pcRight, size_t xLength)  // 길이 제한 문자열 비교 함수를 정의합니다.
{  // strncmp 본문을 시작합니다.
    while ((xLength > 0U) && (*pcLeft != '\0') && (*pcLeft == *pcRight))  // 길이/종단/일치 조건을 확인합니다.
    {  // 비교 반복문 본문을 시작합니다.
        pcLeft++;   // 왼쪽 포인터를 전진시킵니다.
        pcRight++;  // 오른쪽 포인터를 전진시킵니다.
        xLength--;  // 남은 비교 길이를 감소시킵니다.
    }  // 비교 반복문을 종료합니다.

    if (xLength == 0U)  // 지정 길이를 모두 비교했는지 확인합니다.
    {  // 길이 소진 처리 블록을 시작합니다.
        return 0;  // 지정 길이 안에서는 같으므로 0 을 반환합니다.
    }  // 길이 소진 처리 블록을 종료합니다.

    return (int)(unsigned char)*pcLeft - (int)(unsigned char)*pcRight;  // 첫 불일치 문자의 차이를 반환합니다.
}  // strncmp 를 종료합니다.

// --------------------------------------------------------------------------
// C 런타임 초기화
//
// startup_stm32f446xx.s 의 Reset_Handler 는 .data/.bss 를 정리한 뒤
// __libc_init_array() 를 호출합니다. 원래 newlib 가 제공하지만 -nostdlib 이므로
// 여기서 직접 구현합니다. 링커 스크립트가 만든 .preinit_array/.init_array 를
// 순회하며 등록된 초기화 함수를 호출합니다.
// (순수 C 프로젝트라 배열이 비어 있는 것이 정상입니다. 그래도 startup 이
//  이 심볼을 참조하므로 반드시 정의되어야 링크가 됩니다.)
// --------------------------------------------------------------------------
extern void (*__preinit_array_start[])(void) __attribute__((weak));  // preinit 배열의 시작을 외부 선언합니다.
extern void (*__preinit_array_end[])(void) __attribute__((weak));    // preinit 배열의 끝을 외부 선언합니다.
extern void (*__init_array_start[])(void) __attribute__((weak));     // init 배열의 시작을 외부 선언합니다.
extern void (*__init_array_end[])(void) __attribute__((weak));       // init 배열의 끝을 외부 선언합니다.

void __libc_init_array(void)  // C 런타임 초기화 배열 실행 함수를 정의합니다.
{  // __libc_init_array 본문을 시작합니다.
    size_t xCount;  // 배열의 원소 개수를 담을 변수입니다.
    size_t xIndex;  // 배열 순회 인덱스입니다.

    xCount = (size_t)(__preinit_array_end - __preinit_array_start);  // preinit 배열의 원소 수를 계산합니다.
    for (xIndex = 0U; xIndex < xCount; xIndex++)  // preinit 배열 원소를 모두 순회합니다.
    {  // preinit 실행 반복문 본문을 시작합니다.
        __preinit_array_start[xIndex]();  // 등록된 preinit 함수를 호출합니다.
    }  // preinit 실행 반복문을 종료합니다.

    xCount = (size_t)(__init_array_end - __init_array_start);  // init 배열의 원소 수를 계산합니다.
    for (xIndex = 0U; xIndex < xCount; xIndex++)  // init 배열 원소를 모두 순회합니다.
    {  // init 실행 반복문 본문을 시작합니다.
        __init_array_start[xIndex]();  // 등록된 init 함수를 호출합니다.
    }  // init 실행 반복문을 종료합니다.
}  // __libc_init_array 를 종료합니다.
