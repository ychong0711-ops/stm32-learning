// tiny_printf.c — 정수 전용 최소 printf 구현입니다.
//
// [왜 필요한가]
//   main_fixed.c 의 vTask_Debug 는 printf() 로 JSON 로그를 출력합니다.
//   -nostdlib 로 링크하므로 newlib 의 printf 가 없고, 있더라도 다음 문제가 있습니다.
//     - vsnprintf 계열은 재진입 구조체(_impure_ptr)와 힙(malloc)을 사용해
//       FreeRTOS 태스크에서 쓰려면 별도의 재진입 처리가 필요합니다.
//     - 스택 소모가 커서(수백 바이트) 03_static_proof 의 정적 스택 분석 결과를
//       신뢰할 수 없게 만듭니다. (Debug 태스크 스택은 256워드 = 1KB)
//   그래서 필요한 변환만 담은 작은 구현을 두어, 사용하는 코드가 전부 보이고
//   스택 사용량이 상한(TINY_PRINTF_BUF + 소수의 지역 변수)으로 고정되게 합니다.
//
// [지원 범위]
//   %d %i  : 부호 있는 정수      %u : 부호 없는 정수
//   %x %X  : 16진수              %c : 문자          %s : 문자열
//   %p     : 포인터              %% : 퍼센트 문자
//   길이 수식자 l, ll (main_fixed.c 가 %lu, %ld 를 사용)
//   폭 지정과 '0' 채움 (예: %08lX)
//   부동소수점(%f)은 의도적으로 지원하지 않습니다. main_fixed.c 는 이미 온도를
//   0.01℃ 단위 정수(temp_x100)로 바꾸어 출력하므로 필요가 없습니다.
//
// [스레드 안전성]
//   이 구현은 스레드 안전하지 않습니다. main_fixed.c 는 xMutex_Debug 로
//   printf() 호출을 감싸므로 동시 진입이 없습니다. ISR 에서는 호출하지 마십시오.
#include <stdarg.h>  // 가변 인자 처리를 위해 포함합니다.
#include <stddef.h>  // size_t 를 사용하기 위해 포함합니다.
#include <stdint.h>  // 고정 폭 정수 타입을 사용하기 위해 포함합니다.

#define TINY_PRINTF_BUF   24U  // 64비트 정수를 8진수로 표현해도 담기는 변환 버퍼 크기입니다.

// bsp_uart.c 가 제공하는 저수준 출력입니다. (USART2 블로킹 전송)
extern int _write(int iFile, char *pcData, int iLength);  // 저수준 출력 함수를 외부 선언합니다.

// 문자 하나를 출력하고 누적 출력 길이를 갱신하는 내부 헬퍼입니다.
static void prvPutChar(char cValue, int *piCount)  // 문자 출력 헬퍼를 정의합니다.
{  // 문자 출력 헬퍼 본문을 시작합니다.
    (void)_write(1, &cValue, 1);  // 문자 하나를 표준 출력(UART)으로 보냅니다.
    (*piCount)++;  // 출력한 문자 수를 하나 증가시킵니다.
}  // 문자 출력 헬퍼를 종료합니다.

// 부호 없는 정수를 지정 진법 문자열로 변환해 출력합니다.
static void prvPutUnsigned(unsigned long long ullValue,  // 출력할 값입니다.
                           unsigned int uiBase,          // 진법(10 또는 16)입니다.
                           int iUpperCase,               // 16진수를 대문자로 쓸지 여부입니다.
                           unsigned int uiWidth,         // 최소 출력 폭입니다.
                           int iZeroPad,                 // 0 으로 채울지 여부입니다.
                           int *piCount)                 // 누적 출력 길이 포인터입니다.
{  // 부호 없는 정수 출력 함수 본문을 시작합니다.
    char cBuffer[TINY_PRINTF_BUF];  // 자릿수를 역순으로 담을 버퍼입니다.
    const char *pcDigits = iUpperCase ? "0123456789ABCDEF" : "0123456789abcdef";  // 사용할 숫자 문자표입니다.
    unsigned int uiLength = 0U;  // 변환된 자릿수를 세는 변수입니다.

    if (ullValue == 0ULL)  // 값이 0 인 특수 경우인지 확인합니다.
    {  // 0 처리 블록을 시작합니다.
        cBuffer[uiLength++] = '0';  // 문자 '0' 하나를 넣습니다.
    }  // 0 처리 블록을 종료합니다.
    else  // 값이 0 이 아닌 일반적인 경우입니다.
    {  // 일반 변환 블록을 시작합니다.
        while ((ullValue != 0ULL) && (uiLength < TINY_PRINTF_BUF))  // 값이 남아 있고 버퍼에 공간이 있는 동안 반복합니다.
        {  // 자릿수 변환 반복문 본문을 시작합니다.
            cBuffer[uiLength++] = pcDigits[ullValue % (unsigned long long)uiBase];  // 가장 낮은 자릿수를 문자로 변환합니다.
            ullValue /= (unsigned long long)uiBase;  // 값을 진법으로 나누어 다음 자릿수로 넘어갑니다.
        }  // 자릿수 변환 반복문을 종료합니다.
    }  // 일반 변환 블록을 종료합니다.

    while (uiWidth > uiLength)  // 지정 폭에 미치지 못하는 동안 채움 문자를 출력합니다.
    {  // 폭 채움 반복문 본문을 시작합니다.
        prvPutChar(iZeroPad ? '0' : ' ', piCount);  // 0 또는 공백으로 채웁니다.
        uiWidth--;  // 남은 채움 폭을 감소시킵니다.
    }  // 폭 채움 반복문을 종료합니다.

    while (uiLength > 0U)  // 변환된 자릿수가 남아 있는 동안 반복합니다.
    {  // 자릿수 출력 반복문 본문을 시작합니다.
        prvPutChar(cBuffer[--uiLength], piCount);  // 버퍼를 역순으로 읽어 출력합니다.
    }  // 자릿수 출력 반복문을 종료합니다.
}  // 부호 없는 정수 출력 함수를 종료합니다.

int printf(const char *pcFormat, ...)  // 최소 printf 함수를 정의합니다.
{  // printf 본문을 시작합니다.
    va_list xArgs;  // 가변 인자 목록입니다.
    int iCount = 0;  // 출력한 문자 수입니다.
    const char *pcCursor = pcFormat;  // 포맷 문자열을 순회할 커서입니다.

    // GCC 는 printf 의 첫 인자에 nonnull 속성이 있다고 알고 있어서, 이 검사를
    // "항상 참이니 지워도 된다"고 보고 -Wnonnull-compare 경고를 냅니다.
    // 그래도 검사를 남기는 이유는, 여기가 펌웨어의 마지막 로그 경로이기 때문입니다.
    // 다른 곳이 이미 망가져서 NULL 이 흘러들어온 상황에서 하드폴트로 죽는 것보다
    // 조용히 아무것도 출력하지 않는 편이 낫습니다. 그래서 경고만 국소적으로 끕니다.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnonnull-compare"
    if (pcFormat == NULL)  // 포맷 문자열이 유효한지 확인합니다.
    {  // NULL 포맷 처리 블록을 시작합니다.
        return 0;  // 출력한 문자가 없음을 반환합니다.
    }  // NULL 포맷 처리 블록을 종료합니다.
#pragma GCC diagnostic pop

    va_start(xArgs, pcFormat);  // 가변 인자 순회를 시작합니다.

    while (*pcCursor != '\0')  // 포맷 문자열의 끝까지 반복합니다.
    {  // 포맷 순회 반복문 본문을 시작합니다.
        unsigned int uiWidth = 0U;  // 이번 변환의 최소 출력 폭입니다.
        int iZeroPad = 0;  // 0 채움 여부입니다.
        int iLongCount = 0;  // l 수식자의 개수입니다. (0=int, 1=long, 2=long long)

        if (*pcCursor != '%')  // 일반 문자인지 확인합니다.
        {  // 일반 문자 처리 블록을 시작합니다.
            prvPutChar(*pcCursor++, &iCount);  // 문자를 그대로 출력합니다.
            continue;  // 다음 문자로 넘어갑니다.
        }  // 일반 문자 처리 블록을 종료합니다.

        pcCursor++;  // '%' 다음 문자로 이동합니다.

        if (*pcCursor == '0')  // 0 채움 플래그인지 확인합니다.
        {  // 0 채움 처리 블록을 시작합니다.
            iZeroPad = 1;  // 0 으로 채우도록 표시합니다.
            pcCursor++;  // 다음 문자로 이동합니다.
        }  // 0 채움 처리 블록을 종료합니다.

        while ((*pcCursor >= '0') && (*pcCursor <= '9'))  // 폭 지정 숫자가 이어지는 동안 반복합니다.
        {  // 폭 파싱 반복문 본문을 시작합니다.
            uiWidth = (uiWidth * 10U) + (unsigned int)(*pcCursor - '0');  // 자릿수를 폭 값에 누적합니다.
            pcCursor++;  // 다음 문자로 이동합니다.
        }  // 폭 파싱 반복문을 종료합니다.

        while (*pcCursor == 'l')  // 길이 수식자 'l' 이 이어지는 동안 반복합니다.
        {  // 길이 수식자 파싱 반복문 본문을 시작합니다.
            iLongCount++;  // 'l' 개수를 증가시킵니다.
            pcCursor++;  // 다음 문자로 이동합니다.
        }  // 길이 수식자 파싱 반복문을 종료합니다.

        if ((*pcCursor == 'h') || (*pcCursor == 'z'))  // h/z 수식자는 승격 규칙상 int 와 동일하게 처리합니다.
        {  // 수식자 건너뛰기 블록을 시작합니다.
            pcCursor++;  // 수식자 문자를 건너뜁니다.
        }  // 수식자 건너뛰기 블록을 종료합니다.

        switch (*pcCursor)  // 변환 지정자에 따라 분기합니다.
        {  // 변환 지정자 switch 블록을 시작합니다.
            case 'd':  // 부호 있는 10진수 변환입니다.
            case 'i':  // 'i' 는 'd' 와 동일한 변환입니다.
            {  // 부호 있는 정수 처리 블록을 시작합니다.
                long long llValue;  // 부호 있는 값을 담을 변수입니다.

                if (iLongCount >= 2)  // long long 인자인지 확인합니다.
                {  // long long 처리 블록을 시작합니다.
                    llValue = va_arg(xArgs, long long);  // long long 인자를 읽습니다.
                }  // long long 처리 블록을 종료합니다.
                else if (iLongCount == 1)  // long 인자인지 확인합니다.
                {  // long 처리 블록을 시작합니다.
                    llValue = (long long)va_arg(xArgs, long);  // long 인자를 읽습니다.
                }  // long 처리 블록을 종료합니다.
                else  // 기본 int 인자인 경우입니다.
                {  // int 처리 블록을 시작합니다.
                    llValue = (long long)va_arg(xArgs, int);  // int 인자를 읽습니다.
                }  // int 처리 블록을 종료합니다.

                if (llValue < 0)  // 값이 음수인지 확인합니다.
                {  // 음수 처리 블록을 시작합니다.
                    prvPutChar('-', &iCount);  // 부호를 먼저 출력합니다.
                    if (uiWidth > 0U)  // 폭이 지정되어 있는지 확인합니다.
                    {  // 폭 보정 블록을 시작합니다.
                        uiWidth--;  // 부호가 차지한 만큼 폭을 줄입니다.
                    }  // 폭 보정 블록을 종료합니다.
                    // 음수를 부호 없는 값으로 바꿉니다. LLONG_MIN 도 안전하게 처리됩니다.
                    prvPutUnsigned((unsigned long long)(-(llValue + 1)) + 1ULL, 10U, 0, uiWidth, iZeroPad, &iCount);  // 절댓값을 출력합니다.
                }  // 음수 처리 블록을 종료합니다.
                else  // 값이 0 이상인 경우입니다.
                {  // 양수 처리 블록을 시작합니다.
                    prvPutUnsigned((unsigned long long)llValue, 10U, 0, uiWidth, iZeroPad, &iCount);  // 값을 그대로 출력합니다.
                }  // 양수 처리 블록을 종료합니다.
                break;  // 부호 있는 정수 케이스를 종료합니다.
            }  // 부호 있는 정수 처리 블록을 종료합니다.

            case 'u':  // 부호 없는 10진수 변환입니다.
            case 'x':  // 소문자 16진수 변환입니다.
            case 'X':  // 대문자 16진수 변환입니다.
            {  // 부호 없는 정수 처리 블록을 시작합니다.
                unsigned long long ullValue;  // 부호 없는 값을 담을 변수입니다.
                unsigned int uiBase = (*pcCursor == 'u') ? 10U : 16U;  // 변환 진법을 정합니다.

                if (iLongCount >= 2)  // unsigned long long 인자인지 확인합니다.
                {  // unsigned long long 처리 블록을 시작합니다.
                    ullValue = va_arg(xArgs, unsigned long long);  // unsigned long long 인자를 읽습니다.
                }  // unsigned long long 처리 블록을 종료합니다.
                else if (iLongCount == 1)  // unsigned long 인자인지 확인합니다.
                {  // unsigned long 처리 블록을 시작합니다.
                    ullValue = (unsigned long long)va_arg(xArgs, unsigned long);  // unsigned long 인자를 읽습니다.
                }  // unsigned long 처리 블록을 종료합니다.
                else  // 기본 unsigned int 인자인 경우입니다.
                {  // unsigned int 처리 블록을 시작합니다.
                    ullValue = (unsigned long long)va_arg(xArgs, unsigned int);  // unsigned int 인자를 읽습니다.
                }  // unsigned int 처리 블록을 종료합니다.

                prvPutUnsigned(ullValue, uiBase, (*pcCursor == 'X'), uiWidth, iZeroPad, &iCount);  // 값을 출력합니다.
                break;  // 부호 없는 정수 케이스를 종료합니다.
            }  // 부호 없는 정수 처리 블록을 종료합니다.

            case 'c':  // 문자 변환입니다.
            {  // 문자 처리 블록을 시작합니다.
                char cValue = (char)va_arg(xArgs, int);  // 승격된 int 에서 문자를 읽습니다.
                prvPutChar(cValue, &iCount);  // 문자를 출력합니다.
                break;  // 문자 케이스를 종료합니다.
            }  // 문자 처리 블록을 종료합니다.

            case 's':  // 문자열 변환입니다.
            {  // 문자열 처리 블록을 시작합니다.
                const char *pcValue = va_arg(xArgs, const char *);  // 문자열 인자를 읽습니다.

                if (pcValue == NULL)  // NULL 포인터인지 확인합니다.
                {  // NULL 문자열 처리 블록을 시작합니다.
                    pcValue = "(null)";  // 널 표시 문자열로 대체합니다.
                }  // NULL 문자열 처리 블록을 종료합니다.

                while (*pcValue != '\0')  // 문자열 끝까지 반복합니다.
                {  // 문자열 출력 반복문 본문을 시작합니다.
                    prvPutChar(*pcValue++, &iCount);  // 문자를 하나씩 출력합니다.
                }  // 문자열 출력 반복문을 종료합니다.
                break;  // 문자열 케이스를 종료합니다.
            }  // 문자열 처리 블록을 종료합니다.

            case 'p':  // 포인터 변환입니다.
            {  // 포인터 처리 블록을 시작합니다.
                uintptr_t uxValue = (uintptr_t)va_arg(xArgs, void *);  // 포인터 인자를 정수로 읽습니다.
                prvPutChar('0', &iCount);  // 접두사 '0' 을 출력합니다.
                prvPutChar('x', &iCount);  // 접두사 'x' 를 출력합니다.
                prvPutUnsigned((unsigned long long)uxValue, 16U, 0, 8U, 1, &iCount);  // 8자리 0채움 16진수로 출력합니다.
                break;  // 포인터 케이스를 종료합니다.
            }  // 포인터 처리 블록을 종료합니다.

            case '%':  // 퍼센트 문자 자체를 출력하는 변환입니다.
            {  // 퍼센트 처리 블록을 시작합니다.
                prvPutChar('%', &iCount);  // '%' 를 출력합니다.
                break;  // 퍼센트 케이스를 종료합니다.
            }  // 퍼센트 처리 블록을 종료합니다.

            case '\0':  // 포맷 문자열이 '%' 로 끝난 잘못된 경우입니다.
            {  // 조기 종료 처리 블록을 시작합니다.
                va_end(xArgs);  // 가변 인자 순회를 정리합니다.
                return iCount;  // 지금까지 출력한 문자 수를 반환합니다.
            }  // 조기 종료 처리 블록을 종료합니다.

            default:  // 지원하지 않는 변환 지정자인 경우입니다.
            {  // 미지원 지정자 처리 블록을 시작합니다.
                // %f 등 미지원 지정자는 인자를 소비하지 않고 원문 그대로 출력합니다.
                // (인자를 잘못 소비해 이후 변환이 전부 어긋나는 것보다 안전합니다)
                prvPutChar('%', &iCount);  // '%' 를 출력합니다.
                prvPutChar(*pcCursor, &iCount);  // 지정자 문자를 그대로 출력합니다.
                break;  // 미지원 지정자 케이스를 종료합니다.
            }  // 미지원 지정자 처리 블록을 종료합니다.
        }  // 변환 지정자 switch 블록을 종료합니다.

        pcCursor++;  // 변환 지정자 다음 문자로 이동합니다.
    }  // 포맷 순회 반복문을 종료합니다.

    va_end(xArgs);  // 가변 인자 순회를 정리합니다.
    return iCount;  // 출력한 총 문자 수를 반환합니다.
}  // printf 를 종료합니다.

int puts(const char *pcStr)  // puts 함수를 정의합니다. (컴파일러가 printf("...\n") 를 puts 로 치환할 수 있음)
{  // puts 본문을 시작합니다.
    int iCount = 0;  // 출력한 문자 수입니다.

    // printf 와 같은 이유로 NULL 검사를 유지하고 경고만 끕니다. (위 설명 참조)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wnonnull-compare"
    if (pcStr != NULL)  // 문자열 포인터가 유효한지 확인합니다.
    {  // 문자열 출력 블록을 시작합니다.
        while (*pcStr != '\0')  // 문자열 끝까지 반복합니다.
        {  // 문자 출력 반복문 본문을 시작합니다.
            prvPutChar(*pcStr++, &iCount);  // 문자를 하나씩 출력합니다.
        }  // 문자 출력 반복문을 종료합니다.
    }  // 문자열 출력 블록을 종료합니다.
#pragma GCC diagnostic pop

    prvPutChar('\n', &iCount);  // puts 규격대로 줄바꿈을 덧붙입니다.
    return iCount;  // 출력한 문자 수를 반환합니다.
}  // puts 를 종료합니다.

int putchar(int iChar)  // putchar 함수를 정의합니다. (printf("x") 가 putchar 로 치환될 수 있음)
{  // putchar 본문을 시작합니다.
    int iCount = 0;  // 출력한 문자 수입니다.

    prvPutChar((char)iChar, &iCount);  // 문자를 출력합니다.
    return iChar;  // 출력한 문자를 반환합니다.
}  // putchar 를 종료합니다.
