// bsp_baremetal.h — FreeRTOS 없이 BSP 를 빌드할 때 필요한 최소 타입/상수 정의입니다.
// 부트로더(섹터 0)는 커널 없이 동작하므로 FreeRTOS.h 대신 이 헤더를 사용합니다.
// 애플리케이션 빌드에서는 포함되지 않으며, 값은 FreeRTOS 의 정의와 동일하게 맞춥니다.
#ifndef BSP_BAREMETAL_H  // BSP_BAREMETAL_H 가 아직 정의되지 않았는지 확인합니다.
#define BSP_BAREMETAL_H  // BSP_BAREMETAL_H 를 정의하여 중복 포함을 방지합니다.

#include <stdint.h>  // int32_t 등 고정 폭 정수 타입을 사용하기 위해 포함합니다.

typedef int32_t BaseType_t;  // FreeRTOS 의 BaseType_t 와 동일한 폭의 타입을 정의합니다.

#ifndef pdFALSE  // pdFALSE 가 아직 정의되지 않았는지 확인합니다.
    #define pdFALSE ((BaseType_t)0)  // 거짓 값을 정의합니다.
#endif  // pdFALSE 정의 분기를 종료합니다.

#ifndef pdTRUE  // pdTRUE 가 아직 정의되지 않았는지 확인합니다.
    #define pdTRUE ((BaseType_t)1)  // 참 값을 정의합니다.
#endif  // pdTRUE 정의 분기를 종료합니다.

#ifndef pdPASS  // pdPASS 가 아직 정의되지 않았는지 확인합니다.
    #define pdPASS (pdTRUE)  // 성공 값을 정의합니다.
#endif  // pdPASS 정의 분기를 종료합니다.

#ifndef pdFAIL  // pdFAIL 이 아직 정의되지 않았는지 확인합니다.
    #define pdFAIL (pdFALSE)  // 실패 값을 정의합니다.
#endif  // pdFAIL 정의 분기를 종료합니다.

#endif /* BSP_BAREMETAL_H */  // BSP_BAREMETAL_H 조건부 컴파일 블록을 종료합니다.
