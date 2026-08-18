// 
// FreeRTOS 임베디드 애플리케이션 예제 (수정판)
// --------------------------------------------------------------------------
// [수정 1] 워치독(Watchdog) 단일 실패점 제거
// - 기존: IWDG 피드를 CAN 수신 태스크만 담당 → 펌웨어 업데이트 시 CAN 태스크가
// 정지되면 플래싱 도중 워치독 타임아웃으로 시스템이 리셋되어 펌웨어가
// 손상(벽돌)될 위험이 있었음.
// - 수정: 전용 워치독 태스크(vTask_Watchdog)를 두고, 각 태스크의 하트비트를
// 모두 확인한 뒤에만 IWDG를 피드. 펌웨어 업데이트 중에는 업데이트 플래그로
// 무조건 피드로 전환하고, 플래시 기록 함수 내부에서도 IWDG를 피드.
// 
// [수정 2] 긴급 정지(Emergency Stop) 경로 보장
// - 기존: 긴급 정지가 "큐 전송 → 뮤텍스(5ms 타임아웃) → 액추에이터 태스크" 체인을
// 거치며, 큐 가득 참/뮤텍스 실패 시 조용히 유실될 수 있었음.
// - 수정: 긴급 정지는 CAN 수신 태스크에서 큐를 거치지 않고 즉시 안전 상태를
// 적용(prvEmergencyStop). 뮤텍스는 portMAX_DELAY로 획득을 보장.
// --------------------------------------------------------------------------
// BSP 함수들(CAN_Receive, ADC_ReadChannel, PWM_SetDuty 등)은 실제 드라이버 함수가
// 존재한다고 가정한 코드입니다.
// 

#include "FreeRTOS.h" // FreeRTOS 커널 API와 자료형을 사용하기 위해 포함합니다.
#include "task.h" // 태스크 생성, 삭제, 지연, 스케줄링 API를 사용하기 위해 포함합니다.
#include "queue.h" // 태스크 간 데이터 전달을 위한 큐 API를 사용하기 위해 포함합니다.
#include "semphr.h" // 세마포어와 뮤텍스 API를 사용하기 위해 포함합니다.
#include <stdint.h> // uint8_t, uint16_t, uint32_t, int32_t 등 고정 폭 정수 타입을 사용하기 위해 포함합니다.
#include <stdio.h> // printf 기반 UART 디버깅 출력을 사용하기 위해 포함합니다.
#include "bsp_can.h" // CAN 초기화 및 수신 함수가 선언되어 있다고 가정하고 포함합니다.
#include "bsp_adc.h" // ADC 초기화 및 채널 읽기 함수가 선언되어 있다고 가정하고 포함합니다.
#include "bsp_pwm.h" // PWM 초기화 및 듀티 설정 함수가 선언되어 있다고 가정하고 포함합니다.
#include "bsp_gpio.h" // GPIO 초기화 및 출력 제어 함수가 선언되어 있다고 가정하고 포함합니다.
#include "bsp_uart.h" // UART 초기화 함수가 선언되어 있다고 가정하고 포함합니다.
#include "bsp_iwdg.h" // 독립 워치독 피드 함수가 선언되어 있다고 가정하고 포함합니다.
#include "bsp_flash.h" // 내부/외부 플래시 제어 함수가 선언되어 있다고 가정하고 포함합니다.
#include "bootloader.h" // 부트로더 업데이트 요청 확인 및 플래싱 함수가 선언되어 있다고 가정하고 포함합니다.
#include "sensor_bmp280.h" // BMP280 온도/압력 센서 함수가 선언되어 있다고 가정하고 포함합니다.
#include "system_stm32.h" // 시스템 클록 설정 및 NVIC 시스템 리셋 함수가 선언되어 있다고 가정하고 포함합니다.

#define TASK_WATCHDOG_STACK_SIZE 128U // 워치독 태스크의 스택 크기를 워드 단위로 정의합니다.
#define TASK_CAN_STACK_SIZE 256U // CAN 수신 태스크의 스택 크기를 워드 단위로 정의합니다.
#define TASK_SENSOR_STACK_SIZE 128U // 센서 수집 태스크의 스택 크기를 워드 단위로 정의합니다.
#define TASK_ACTUATOR_STACK_SIZE 128U // 액추에이터 제어 태스크의 스택 크기를 워드 단위로 정의합니다.
#define TASK_DEBUG_STACK_SIZE 256U // 디버깅 태스크의 스택 크기를 워드 단위로 정의합니다.
#define TASK_FIRMWARE_STACK_SIZE 512U // 펌웨어 업데이트 태스크의 스택 크기를 워드 단위로 정의합니다.

#define TASK_WATCHDOG_PRIORITY 6U // 워치독 태스크는 최상위 우선순위로 두어 항상 실행되도록 합니다.
#define TASK_CAN_PRIORITY 5U // CAN 수신 태스크는 두 번째로 높은 우선순위로 설정합니다.
#define TASK_SENSOR_PRIORITY 3U // 센서 태스크는 중간 우선순위로 설정합니다.
#define TASK_ACTUATOR_PRIORITY 3U // 액추에이터 태스크는 중간 우선순위로 설정합니다.
#define TASK_DEBUG_PRIORITY 2U // 디버깅 태스크는 낮은 우선순위로 설정합니다.
#define TASK_FIRMWARE_PRIORITY 1U // 펌웨어 업데이트 태스크는 가장 낮은 우선순위로 설정합니다.

#define ACTUATOR_CMD_QUEUE_LEN 16U // 액추에이터 명령 큐의 최대 깊이를 정의합니다.
#define SENSOR_DATA_QUEUE_LEN 16U // 센서 데이터 큐의 최대 깊이를 정의합니다.

#define WATCHDOG_CHECK_PERIOD_MS 100U // 워치독 태스크의 하트비트 검사 주기를 100ms로 정의합니다.
#define CAN_RX_TIMEOUT_MS 10U // CAN 수신 타임아웃을 10ms로 정의합니다.
#define ACTUATOR_QUEUE_TIMEOUT_MS 20U // 액추에이터 명령 큐 대기 타임아웃을 20ms로 정의합니다.
#define ACTUATOR_MUTEX_TIMEOUT_MS 5U // 액추에이터 뮤텍스 획득 타임아웃을 5ms로 정의합니다.
#define DEBUG_QUEUE_TIMEOUT_MS 100U // 디버깅 태스크의 센서 큐 대기 타임아웃을 100ms로 정의합니다.
#define DEBUG_MUTEX_TIMEOUT_MS 50U // 디버깅 뮤텍스 획득 타임아웃을 50ms로 정의합니다.
#define FIRMWARE_CHECK_PERIOD_MS 5000U // 펌웨어 업데이트 요청 확인 주기를 5초로 정의합니다.

#define CAN_ID_EMERGENCY 0x123U // 긴급 정지 메시지로 사용할 CAN ID를 정의합니다.
#define CAN_ID_THROTTLE_CMD 0x200U // 스로틀 제어 명령으로 사용할 CAN ID를 정의합니다.
#define CAN_ID_FAN_CMD 0x201U // 팬 제어 명령으로 사용할 CAN ID를 정의합니다.

#define ADC_CHANNEL_RPM 0U // RPM 센서를 읽는 ADC 채널 번호를 정의합니다.
#define ADC_CHANNEL_THROTTLE 1U // 스로틀 센서를 읽는 ADC 채널 번호를 정의합니다.
#define PWM_CHANNEL_THROTTLE 0U // 스로틀 제어에 사용할 PWM 채널을 정의합니다.
#define GPIO_PIN_FAN 13U // 팬 제어에 사용할 GPIO 핀 번호를 정의합니다.

#define UART_BAUD_115200 115200U // UART 통신 속도를 115200bps로 정의합니다.
#define CAN_SPEED_500KBPS 500000U // CAN 통신 속도를 500kbps로 정의합니다.

// IWDG 리로드 값(타임아웃)은 워치독 검사 주기(100ms)보다 충분히 크게 설정해야 합니다.
// 예: IWDG 프리스케일러/리로드를 조합하여 약 1초로 설정하는 것을 권장합니다. 

typedef struct // CAN 메시지 구조체 정의를 시작합니다.
{ // CAN 메시지 멤버 변수 선언을 시작합니다.
    uint32_t ID; // CAN 메시지 식별자를 저장합니다.
    uint8_t DLC; // CAN 데이터 길이 코드를 저장합니다.
    uint8_t data[8]; // CAN 데이터 페이로드 최대 8바이트를 저장합니다.
} CAN_Message_t; // CAN 메시지 구조체 타입 이름을 CAN_Message_t로 정의합니다.

typedef struct // 센서 데이터 구조체 정의를 시작합니다.
{ // 센서 데이터 멤버 변수 선언을 시작합니다.
    uint16_t rpm; // RPM 센서 측정값을 저장합니다.
    float temperature; // 온도 센서 측정값을 저장합니다.
    uint16_t throttle; // 스로틀 위치 센서 측정값을 저장합니다.
    TickType_t timestamp; // 센서 데이터를 읽은 시점의 FreeRTOS 틱을 저장합니다.
} SensorData_t; // 센서 데이터 구조체 타입 이름을 SensorData_t로 정의합니다.

typedef enum // 액추에이터 명령 ID 열거형 정의를 시작합니다.
{ // 액추에이터 명령 ID 값 선언을 시작합니다.
    CMD_NONE = 0, // 유효하지 않은 명령 또는 명령 없음을 의미합니다.
    CMD_THROTTLE, // 스로틀 PWM 제어 명령을 의미합니다.
    CMD_FAN, // 팬 GPIO 제어 명령을 의미합니다.
    CMD_EMERGENCY_STOP // 긴급 정지 명령을 의미합니다.
} ActuatorCommandId_t; // 액추에이터 명령 ID 타입 이름을 ActuatorCommandId_t로 정의합니다.

typedef struct // 액추에이터 명령 구조체 정의를 시작합니다.
{ // 액추에이터 명령 멤버 변수 선언을 시작합니다.
    ActuatorCommandId_t commandId; // 실행할 액추에이터 명령 종류를 저장합니다.
    uint16_t duty; // PWM 듀티 또는 제어량을 저장합니다.
    uint8_t state; // GPIO 온/오프 상태를 저장합니다.
    uint32_t sourceCanId; // 명령을 발생시킨 CAN ID를 저장합니다.
} ActuatorCmd_t; // 액추에이터 명령 구조체 타입 이름을 ActuatorCmd_t로 정의합니다.

typedef enum // 펌웨어 상태 열거형 정의를 시작합니다.
{ // 펌웨어 상태 값 선언을 시작합니다.
    UPDATE_NONE = 0, // 펌웨어 업데이트 요청이 없는 상태를 의미합니다.
    UPDATE_AVAILABLE // 펌웨어 업데이트 요청이 존재하는 상태를 의미합니다.
} FirmwareState_t; // 펌웨어 상태 타입 이름을 FirmwareState_t로 정의합니다.

static TaskHandle_t xTaskHandle_Watchdog = NULL; // 워치독 태스크의 핸들을 저장합니다.
static TaskHandle_t xTaskHandle_CAN_Rx = NULL; // CAN 수신 태스크의 핸들을 저장합니다.
static TaskHandle_t xTaskHandle_Sensor = NULL; // 센서 수집 태스크의 핸들을 저장합니다.
static TaskHandle_t xTaskHandle_Actuator = NULL; // 액추에이터 제어 태스크의 핸들을 저장합니다.
static TaskHandle_t xTaskHandle_Debug = NULL; // 디버깅 태스크의 핸들을 저장합니다.
static TaskHandle_t xTaskHandle_Firmware = NULL; // 펌웨어 업데이트 태스크의 핸들을 저장합니다.

static QueueHandle_t xQueue_ActuatorCmd = NULL; // 액추에이터 명령을 전달하는 큐 핸들입니다.
static QueueHandle_t xQueue_Sensor_Data = NULL; // 센서 데이터를 전달하는 큐 핸들입니다.

static SemaphoreHandle_t xSemaphore_Actuator = NULL; // 액추에이터 자원 보호용 뮤텍스입니다.
static SemaphoreHandle_t xMutex_Debug = NULL; // UART/printf 동시 접근 방지용 뮤텍스입니다.

// ---- 워치독 하트비트 및 업데이트 플래그 (수정 1) ---- 
static volatile uint32_t ulHeartbeat_CAN_Rx = 0U; // CAN 수신 태스크의 하트비트 카운터입니다.
static volatile uint32_t ulHeartbeat_Sensor = 0U; // 센서 태스크의 하트비트 카운터입니다.
static volatile uint32_t ulHeartbeat_Actuator = 0U; // 액추에이터 태스크의 하트비트 카운터입니다.
static volatile uint32_t ulHeartbeat_Debug = 0U; // 디버깅 태스크의 하트비트 카운터입니다.
static volatile BaseType_t xFirmwareUpdateActive = pdFALSE; // 펌웨어 업데이트 진행 중 여부 플래그입니다.
// 32비트 카운터가 한 검사 주기(100ms) 안에 2^32번 증가할 수 없으므로,
// 이전 값과의 단순 비교만으로도 오버플로(wrap)에 안전합니다. 

// ---- 오류 카운터: 빈 문장(;) 대신 실제 진단용 카운터로 대체 ---- 
static volatile uint32_t ulErrCount_ActuatorQueueFull = 0U; // 액추에이터 명령 큐 가득 참 횟수입니다.
static volatile uint32_t ulErrCount_SensorQueueFull = 0U; // 센서 데이터 큐 가득 참 횟수입니다.
static volatile uint32_t ulErrCount_ActuatorMutexFail = 0U; // 액추에이터 뮤텍스 획득 실패 횟수입니다.
static volatile uint32_t ulErrCount_DebugMutexFail = 0U; // 디버그 뮤텍스 획득 실패 횟수입니다.
static volatile uint32_t ulErrCount_FirmwareStageInvalid = 0U; // 스테이징 이미지 검증 실패 횟수입니다.

void vTask_Watchdog(void *pvParameters); // 워치독 태스크 함수의 프로토타입을 선언합니다.
void vTask_CAN_Rx(void *pvParameters); // CAN 수신 태스크 함수의 프로토타입을 선언합니다.
void vTask_Sensor(void *pvParameters); // 센서 수집 태스크 함수의 프로토타입을 선언합니다.
void vTask_Actuator(void *pvParameters); // 액추에이터 제어 태스크 함수의 프로토타입을 선언합니다.
void vTask_Debug(void *pvParameters); // 디버깅 태스크 함수의 프로토타입을 선언합니다.
void vTask_Firmware(void *pvParameters); // 펌웨어 업데이트 태스크 함수의 프로토타입을 선언합니다.

static void prvConvertCanToActuatorCmd(const CAN_Message_t *pxCanMsg, // CAN 원본 메시지 포인터를 인자로 받습니다.
                                       ActuatorCmd_t *pxCmd); // 변환된 액추에이터 명령을 저장할 포인터를 인자로 받습니다.
static void prvApplyActuatorCommand(const ActuatorCmd_t *pxCmd); // 액추에이터 명령을 실제 하드웨어에 적용하는 함수 프로토타입입니다.
static void prvEmergencyStop(void); // 긴급 정지를 즉시 적용하는 함수 프로토타입입니다.
static void prvRequestFirmwareInstall(void); // 스테이징 이미지를 검증하고 설치를 요청하는 함수 프로토타입입니다.

// --------------------------------------------------------------------------
// 워치독 태스크 (수정 1)
// 모든 태스크의 하트비트가 갱신되고 있을 때만 IWDG를 피드합니다.
// 하나라도 멈춘 태스크가 있으면 피드하지 않아 IWDG가 시스템을 리셋시키도록 합니다.
// 펌웨어 업데이트 중에는 다른 태스크가 정지되므로 업데이트 플래그를 보고 무조건 피드합니다.
// -------------------------------------------------------------------------- 
void vTask_Watchdog(void *pvParameters) // 워치독 태스크 함수를 정의합니다.
{ // 워치독 태스크 함수 본문을 시작합니다.
    (void)pvParameters; // 사용하지 않는 태스크 파라미터로 인한 경고를 방지합니다.
    uint32_t ulPrevHeartbeat_CAN_Rx = 0U; // 이전 주기의 CAN 하트비트 값을 저장합니다.
    uint32_t ulPrevHeartbeat_Sensor = 0U; // 이전 주기의 센서 하트비트 값을 저장합니다.
    uint32_t ulPrevHeartbeat_Actuator = 0U; // 이전 주기의 액추에이터 하트비트 값을 저장합니다.
    uint32_t ulPrevHeartbeat_Debug = 0U; // 이전 주기의 디버그 하트비트 값을 저장합니다.
    const TickType_t xPeriod = pdMS_TO_TICKS(WATCHDOG_CHECK_PERIOD_MS); // 검사 주기 100ms를 틱 단위로 변환합니다.
    TickType_t xLastWakeTime = xTaskGetTickCount(); // 주기 지연을 위해 현재 틱으로 마지막 깨어남 시각을 초기화합니다.

    for (;;) // 워치독 태스크의 무한 루프를 시작합니다.
    { // 워치독 태스크 루프 본문을 시작합니다.
        if (xFirmwareUpdateActive != pdFALSE) // 펌웨어 업데이트가 진행 중인지 확인합니다.
        { // 펌웨어 업데이트 중 처리 블록을 시작합니다.
            IWDG_ReloadCounter(); // 업데이트 중에는 업데이터를 신뢰하고 무조건 IWDG를 피드합니다.
        } // 펌웨어 업데이트 중 처리 블록을 종료합니다.
        else if ((ulHeartbeat_CAN_Rx != ulPrevHeartbeat_CAN_Rx) && // CAN 태스크 하트비트가 갱신되었는지 확인합니다.
                 (ulHeartbeat_Sensor != ulPrevHeartbeat_Sensor) && // 센서 태스크 하트비트가 갱신되었는지 확인합니다.
                 (ulHeartbeat_Actuator != ulPrevHeartbeat_Actuator) && // 액추에이터 태스크 하트비트가 갱신되었는지 확인합니다.
                 (ulHeartbeat_Debug != ulPrevHeartbeat_Debug)) // 디버그 태스크 하트비트가 갱신되었는지 확인합니다.
        { // 모든 태스크 정상 동작 확인 블록을 시작합니다.
            IWDG_ReloadCounter(); // 모든 태스크가 살아있으므로 IWDG를 피드합니다.
        } // 모든 태스크 정상 동작 확인 블록을 종료합니다.
        else // 하나 이상의 태스크가 멈춘 경우입니다.
        { // 태스크 이상 감지 블록을 시작합니다.
            ; // 의도적으로 피드하지 않습니다. IWDG 타임아웃으로 시스템이 리셋됩니다.
              // 실제 제품에서는 리셋 직전에 어떤 태스크가 멈췄는지 UART로 기록하면 좋습니다.
        } // 태스크 이상 감지 블록을 종료합니다.

        ulPrevHeartbeat_CAN_Rx = ulHeartbeat_CAN_Rx; // 다음 비교를 위해 CAN 하트비트 값을 갱신합니다.
        ulPrevHeartbeat_Sensor = ulHeartbeat_Sensor; // 다음 비교를 위해 센서 하트비트 값을 갱신합니다.
        ulPrevHeartbeat_Actuator = ulHeartbeat_Actuator; // 다음 비교를 위해 액추에이터 하트비트 값을 갱신합니다.
        ulPrevHeartbeat_Debug = ulHeartbeat_Debug; // 다음 비교를 위해 디버그 하트비트 값을 갱신합니다.

        vTaskDelayUntil(&xLastWakeTime, xPeriod); // 다음 100ms 검사 주기까지 태스크를 지연시킵니다.
    } // 워치독 태스크 무한 루프를 종료합니다.
} // 워치독 태스크 함수를 종료합니다.

void vTask_CAN_Rx(void *pvParameters) // CAN 수신 태스크 함수를 정의합니다.
{ // CAN 수신 태스크 함수 본문을 시작합니다.
    (void)pvParameters; // 사용하지 않는 태스크 파라미터로 인한 경고를 방지합니다.
    CAN_Message_t xRxMsg; // CAN에서 수신한 메시지를 저장할 변수입니다.
    ActuatorCmd_t xCmd; // CAN 메시지를 변환한 액추에이터 명령을 저장할 변수입니다.
    BaseType_t xStatus; // CAN 수신 성공 여부를 저장할 변수입니다.

    CAN_Init(CAN_SPEED_500KBPS); // CAN 컨트롤러를 500kbps 속도로 초기화합니다.
    CAN_FilterConfig(0x000, 0x000); // 모든 CAN 메시지를 수신하도록 필터를 설정합니다.

    for (;;) // CAN 수신 태스크의 무한 루프를 시작합니다.
    { // CAN 수신 태스크 루프 본문을 시작합니다.
        ulHeartbeat_CAN_Rx++; // 워치독이 이 태스크의 생존을 확인하도록 하트비트를 증가시킵니다.

        xStatus = CAN_Receive(&xRxMsg, pdMS_TO_TICKS(CAN_RX_TIMEOUT_MS)); // 10ms 타임아웃으로 CAN 메시지 수신을 시도합니다.
        if (xStatus == pdPASS) // CAN 메시지 수신에 성공했는지 확인합니다.
        { // CAN 메시지 수신 성공 처리 블록을 시작합니다.
            if (xRxMsg.ID == CAN_ID_EMERGENCY) // 수신된 CAN ID가 긴급 정지 ID인지 확인합니다.
            { // 긴급 정지 처리 블록을 시작합니다.
                prvEmergencyStop(); // (수정 2) 큐를 거치지 않고 즉시 안전 상태를 적용합니다.
            } // 긴급 정지 처리 블록을 종료합니다.
            else // 일반 제어 명령인 경우입니다.
            { // 일반 제어 명령 처리 블록을 시작합니다.
                prvConvertCanToActuatorCmd(&xRxMsg, &xCmd); // 수신된 CAN 메시지를 액추에이터 명령 구조체로 변환합니다.
                if (xCmd.commandId != CMD_NONE) // 변환된 명령이 실제 제어 대상 명령인지 확인합니다.
                { // 액추에이터 명령 처리 블록을 시작합니다.
                    if (xQueueSend(xQueue_ActuatorCmd, &xCmd, 0U) != pdPASS) // 액추에이터 명령 큐에 명령 전송을 시도하고 실패 여부를 확인합니다.
                    { // 액추에이터 명령 큐 전송 실패 처리 블록을 시작합니다.
                        ulErrCount_ActuatorQueueFull++; // 큐 가득 참으로 명령이 유실된 횟수를 기록합니다.
                    } // 액추에이터 명령 큐 전송 실패 처리 블록을 종료합니다.
                } // 액추에이터 명령 처리 블록을 종료합니다.
            } // 일반 제어 명령 처리 블록을 종료합니다.
        } // CAN 메시지 수신 성공 처리 블록을 종료합니다.
        // IWDG 피드는 이 태스크가 아닌 워치독 전용 태스크가 담당합니다. 
    } // CAN 수신 태스크 무한 루프를 종료합니다.
} // CAN 수신 태스크 함수를 종료합니다.

void vTask_Sensor(void *pvParameters) // 센서 수집 태스크 함수를 정의합니다.
{ // 센서 수집 태스크 함수 본문을 시작합니다.
    (void)pvParameters; // 사용하지 않는 태스크 파라미터로 인한 경고를 방지합니다.
    SensorData_t xSensorData; // 한 주기 동안 읽은 센서 데이터를 저장할 변수입니다.
    const TickType_t xPeriod = pdMS_TO_TICKS(10U); // 센서 읽기 주기 10ms를 틱 단위로 변환하여 저장합니다.
    TickType_t xLastWakeTime = xTaskGetTickCount(); // 주기 지연을 위해 현재 틱 값으로 마지막 깨어남 시각을 초기화합니다.

    ADC_Init(); // ADC 주변장치를 초기화합니다.
    BMP280_Init(); // BMP280 온도/압력 센서를 초기화합니다.

    for (;;) // 센서 수집 태스크의 무한 루프를 시작합니다.
    { // 센서 수집 태스크 루프 본문을 시작합니다.
        ulHeartbeat_Sensor++; // 워치독이 이 태스크의 생존을 확인하도록 하트비트를 증가시킵니다.

        xSensorData.rpm = ADC_ReadChannel(ADC_CHANNEL_RPM); // RPM 센서 값을 ADC로 읽습니다.
        xSensorData.temperature = BMP280_ReadTemperature(); // 온도 센서 값을 BMP280에서 읽습니다.
        xSensorData.throttle = ADC_ReadChannel(ADC_CHANNEL_THROTTLE); // 스로틀 센서 값을 ADC로 읽습니다.
        xSensorData.timestamp = xTaskGetTickCount(); // 센서 읽기 시점의 타임스탬프를 저장합니다.

        if (xQueueSend(xQueue_Sensor_Data, &xSensorData, 0U) != pdPASS) // 센서 데이터 큐에 데이터 전송을 시도하고 실패 여부를 확인합니다.
        { // 센서 데이터 큐 전송 실패 처리 블록을 시작합니다.
            ulErrCount_SensorQueueFull++; // 큐 가득 참으로 센서 데이터가 유실된 횟수를 기록합니다.
        } // 센서 데이터 큐 전송 실패 처리 블록을 종료합니다.

        vTaskDelayUntil(&xLastWakeTime, xPeriod); // 다음 10ms 주기까지 태스크를 지연시켜 정밀한 주기를 유지합니다.
    } // 센서 수집 태스크 무한 루프를 종료합니다.
} // 센서 수집 태스크 함수를 종료합니다.

void vTask_Actuator(void *pvParameters) // 액추에이터 제어 태스크 함수를 정의합니다.
{ // 액추에이터 제어 태스크 함수 본문을 시작합니다.
    (void)pvParameters; // 사용하지 않는 태스크 파라미터로 인한 경고를 방지합니다.
    ActuatorCmd_t xCmd; // 큐에서 수신한 액추에이터 명령을 저장할 변수입니다.
    BaseType_t xStatus; // 명령 큐 수신 성공 여부를 저장할 변수입니다.
    const TickType_t xQueueTimeout = pdMS_TO_TICKS(ACTUATOR_QUEUE_TIMEOUT_MS); // 명령 큐 대기 타임아웃을 20ms로 설정합니다.

    PWM_Init(PWM_CHANNEL_THROTTLE, 20000U); // 스로틀 PWM 채널을 20kHz로 초기화합니다.
    GPIO_Init(GPIO_PIN_FAN); // 팬 제어용 GPIO 핀을 초기화합니다.
    PWM_SetDuty(PWM_CHANNEL_THROTTLE, 0U); // 초기 상태에서 스로틀 PWM 듀티를 0으로 설정합니다.
    GPIO_Write(GPIO_PIN_FAN, 0U); // 초기 상태에서 팬 GPIO를 OFF로 설정합니다.

    for (;;) // 액추에이터 제어 태스크의 무한 루프를 시작합니다.
    { // 액추에이터 제어 태스크 루프 본문을 시작합니다.
        ulHeartbeat_Actuator++; // 워치독이 이 태스크의 생존을 확인하도록 하트비트를 증가시킵니다.

        xStatus = xQueueReceive(xQueue_ActuatorCmd, &xCmd, xQueueTimeout); // 액추에이터 명령 큐에서 명령 수신을 시도합니다.
        if (xStatus == pdPASS) // 명령 큐에서 명령을 정상적으로 수신했는지 확인합니다.
        { // 명령 수신 성공 처리 블록을 시작합니다.
            if (xSemaphoreTake(xSemaphore_Actuator, pdMS_TO_TICKS(ACTUATOR_MUTEX_TIMEOUT_MS)) == pdTRUE) // 액추에이터 뮤텍스 획득을 시도하고 성공 여부를 확인합니다.
            { // 액추에이터 뮤텍스 획득 성공 블록을 시작합니다.
                prvApplyActuatorCommand(&xCmd); // 수신된 명령을 실제 PWM/GPIO 출력에 적용합니다.
                xSemaphoreGive(xSemaphore_Actuator); // 사용이 끝난 액추에이터 뮤텍스를 반환합니다.
            } // 액추에이터 뮤텍스 획득 성공 블록을 종료합니다.
            else // 액추에이터 뮤텍스 획득에 실패한 경우를 처리합니다.
            { // 액추에이터 뮤텍스 획득 실패 블록을 시작합니다.
                ulErrCount_ActuatorMutexFail++; // 뮤텍스 획득 실패로 명령이 누락된 횟수를 기록합니다.
            } // 액추에이터 뮤텍스 획득 실패 블록을 종료합니다.
        } // 명령 수신 성공 처리 블록을 종료합니다.
        // 긴급 정지는 이 태스크가 아닌 CAN 수신 태스크에서 즉시 처리됩니다. 
    } // 액추에이터 제어 태스크 무한 루프를 종료합니다.
} // 액추에이터 제어 태스크 함수를 종료합니다.

static void prvConvertCanToActuatorCmd(const CAN_Message_t *pxCanMsg, // CAN 원본 메시지 포인터를 인자로 받습니다.
                                       ActuatorCmd_t *pxCmd) // 변환된 명령을 저장할 포인터를 인자로 받습니다.
{ // CAN 메시지 변환 함수 본문을 시작합니다.
    pxCmd->commandId = CMD_NONE; // 기본 명령 ID를 명령 없음으로 초기화합니다.
    pxCmd->duty = 0U; // 기본 듀티 값을 0으로 초기화합니다.
    pxCmd->state = 0U; // 기본 GPIO 상태 값을 0으로 초기화합니다.
    pxCmd->sourceCanId = pxCanMsg->ID; // 명령 출처 CAN ID를 저장합니다.

    switch (pxCanMsg->ID) // CAN ID에 따라 명령 종류를 결정합니다.
    { // CAN ID switch 블록을 시작합니다.
        case CAN_ID_THROTTLE_CMD: // CAN ID가 스로틀 제어 명령인 경우입니다.
            if (pxCanMsg->DLC >= 2U) // 스로틀 명령에 필요한 데이터 길이가 최소 2바이트인지 확인합니다.
            { // 스로틀 데이터 처리 블록을 시작합니다.
                pxCmd->commandId = CMD_THROTTLE; // 명령 ID를 스로틀 명령으로 설정합니다.
                pxCmd->duty = (uint16_t)(((uint16_t)pxCanMsg->data[1] << 8) | // data[1]을 상위 바이트로 사용하기 위해 8비트 왼쪽 시프트합니다.
                                          (uint16_t)pxCanMsg->data[0]); // data[0]을 하위 바이트로 결합하여 16비트 듀티 값을 만듭니다.
            } // 스로틀 데이터 처리 블록을 종료합니다.
            break; // 스로틀 명령 케이스 처리를 종료합니다.

        case CAN_ID_FAN_CMD: // CAN ID가 팬 제어 명령인 경우입니다.
            if (pxCanMsg->DLC >= 1U) // 팬 명령에 필요한 데이터 길이가 최소 1바이트인지 확인합니다.
            { // 팬 데이터 처리 블록을 시작합니다.
                pxCmd->commandId = CMD_FAN; // 명령 ID를 팬 명령으로 설정합니다.
                pxCmd->state = pxCanMsg->data[0]; // data[0]을 팬 온/오프 상태로 설정합니다.
            } // 팬 데이터 처리 블록을 종료합니다.
            break; // 팬 명령 케이스 처리를 종료합니다.

        case CAN_ID_EMERGENCY: // CAN ID가 긴급 정지 명령인 경우입니다.
            // 긴급 정지는 CAN 수신 태스크에서 prvEmergencyStop()으로 즉시 처리하므로
// 일반적으로 여기 도달하지 않지만, 방어적으로 매핑을 유지합니다. 
            pxCmd->commandId = CMD_EMERGENCY_STOP; // 명령 ID를 긴급 정지로 설정합니다.
            pxCmd->duty = 0U; // 긴급 정지 시 듀티를 0으로 설정합니다.
            pxCmd->state = 0U; // 긴급 정지 시 GPIO 상태를 0으로 설정합니다.
            break; // 긴급 정지 명령 케이스 처리를 종료합니다.

        default: // 위에서 정의되지 않은 CAN ID인 경우입니다.
            break; // 제어 대상이 아니므로 아무 처리 없이 종료합니다.
    } // CAN ID switch 블록을 종료합니다.
} // CAN 메시지 변환 함수를 종료합니다.

static void prvApplyActuatorCommand(const ActuatorCmd_t *pxCmd) // 액추에이터 명령 적용 함수를 정의합니다.
{ // 액추에이터 명령 적용 함수 본문을 시작합니다.
    switch (pxCmd->commandId) // 명령 종류에 따라 실제 하드웨어 출력을 제어합니다.
    { // 명령 switch 블록을 시작합니다.
        case CMD_THROTTLE: // 스로틀 명령인 경우입니다.
            PWM_SetDuty(PWM_CHANNEL_THROTTLE, pxCmd->duty); // 스로틀 PWM 채널에 명령 듀티를 적용합니다.
            break; // 스로틀 명령 처리를 종료합니다.

        case CMD_FAN: // 팬 명령인 경우입니다.
            GPIO_Write(GPIO_PIN_FAN, pxCmd->state); // 팬 GPIO 핀에 명령 상태를 적용합니다.
            break; // 팬 명령 처리를 종료합니다.

        case CMD_EMERGENCY_STOP: // 긴급 정지 명령인 경우입니다.
            PWM_SetDuty(PWM_CHANNEL_THROTTLE, 0U); // 스로틀 PWM을 0으로 설정하여 출력을 차단합니다.
            GPIO_Write(GPIO_PIN_FAN, 0U); // 팬 GPIO를 OFF로 설정합니다.
            break; // 긴급 정지 명령 처리를 종료합니다.

        case CMD_NONE: // 명령 없음인 경우입니다.
        default: // 알 수 없는 명령인 경우입니다.
            break; // 추가 하드웨어 동작 없이 종료합니다.
    } // 명령 switch 블록을 종료합니다.
} // 액추에이터 명령 적용 함수를 종료합니다.

// --------------------------------------------------------------------------
// 긴급 정지 (수정 2)
// 큐를 거치지 않고 호출 즉시 안전 상태를 적용합니다.
// 뮤텍스는 portMAX_DELAY로 획득을 보장하므로 명령이 유실되지 않습니다.
// -------------------------------------------------------------------------- 
static void prvEmergencyStop(void) // 긴급 정지 함수를 정의합니다.
{ // 긴급 정지 함수 본문을 시작합니다.
    if (xSemaphoreTake(xSemaphore_Actuator, portMAX_DELAY) == pdTRUE) // 긴급 경로이므로 무한 대기로 액추에이터 뮤텍스 획득을 보장합니다.
    { // 액추에이터 뮤텍스 획득 성공 블록을 시작합니다.
        PWM_SetDuty(PWM_CHANNEL_THROTTLE, 0U); // 스로틀 PWM을 0으로 설정하여 출력을 차단합니다.
        GPIO_Write(GPIO_PIN_FAN, 0U); // 팬 GPIO를 OFF로 설정합니다.
        xSemaphoreGive(xSemaphore_Actuator); // 사용이 끝난 액추에이터 뮤텍스를 반환합니다.
    } // 액추에이터 뮤텍스 획득 성공 블록을 종료합니다.
    else // 뮤텍스 획득에 실패한 경우입니다. (portMAX_DELAY 사용 시 도달하지 않아야 함)
    { // 뮤텍스 획득 실패 방어 블록을 시작합니다.
        // 이론상 도달하지 않지만, 안전이 최우선이므로 뮤텍스 없이도 안전 출력을 시도합니다. 
        PWM_SetDuty(PWM_CHANNEL_THROTTLE, 0U); // 스로틀 PWM을 0으로 설정합니다.
        GPIO_Write(GPIO_PIN_FAN, 0U); // 팬 GPIO를 OFF로 설정합니다.
    } // 뮤텍스 획득 실패 방어 블록을 종료합니다.
} // 긴급 정지 함수를 종료합니다.

void vTask_Debug(void *pvParameters) // 디버깅 태스크 함수를 정의합니다.
{ // 디버깅 태스크 함수 본문을 시작합니다.
    (void)pvParameters; // 사용하지 않는 태스크 파라미터로 인한 경고를 방지합니다.
    SensorData_t xSensorData; // 디버깅 출력에 사용할 센서 데이터를 저장할 변수입니다.
    BaseType_t xStatus; // 센서 큐 수신 성공 여부를 저장할 변수입니다.
    int32_t lTempX100; // 온도를 0.01℃ 단위 정수로 저장할 변수입니다.

    UART_Init(UART_BAUD_115200); // 디버깅용 UART를 115200bps로 초기화합니다.

    for (;;) // 디버깅 태스크의 무한 루프를 시작합니다.
    { // 디버깅 태스크 루프 본문을 시작합니다.
        ulHeartbeat_Debug++; // 워치독이 이 태스크의 생존을 확인하도록 하트비트를 증가시킵니다.

        xStatus = xQueueReceive(xQueue_Sensor_Data, &xSensorData, pdMS_TO_TICKS(DEBUG_QUEUE_TIMEOUT_MS)); // 센서 데이터 큐에서 데이터 수신을 시도합니다.
        if (xStatus == pdPASS) // 센서 데이터 큐에서 데이터를 정상적으로 수신했는지 확인합니다.
        { // 센서 데이터 수신 성공 처리 블록을 시작합니다.
            if (xSemaphoreTake(xMutex_Debug, pdMS_TO_TICKS(DEBUG_MUTEX_TIMEOUT_MS)) == pdTRUE) // UART 출력 보호용 뮤텍스 획득을 시도합니다.
            { // 디버그 뮤텍스 획득 성공 블록을 시작합니다.
                // newlib-nano에서는 float 출력이 기본 미지원이므로(-u _printf_float 옵션 필요),
// 온도를 0.01℃ 단위 정수로 변환하여 출력합니다. 이렇게 하면 링커 옵션
// 의존성과 float printf의 코드 크기/스택 부담을 모두 제거할 수 있습니다. 
                lTempX100 = (int32_t)(xSensorData.temperature * 100.0f); // 온도(float)를 0.01℃ 단위 정수로 변환합니다.
                printf("{\"ts\":%lu,\"rpm\":%u,\"temp_x100\":%ld,\"throttle\":%u}\r\n", // JSON 형식 로그 포맷 문자열입니다.
                       (unsigned long)xSensorData.timestamp, // 로그의 타임스탬프 인자입니다.
                       (unsigned int)xSensorData.rpm, // 로그의 RPM 인자입니다.
                       (long)lTempX100, // 로그의 온도 인자입니다. (0.01℃ 단위)
                       (unsigned int)xSensorData.throttle); // 로그의 스로틀 인자입니다.
                xSemaphoreGive(xMutex_Debug); // 사용이 끝난 디버그 뮤텍스를 반환합니다.
            } // 디버그 뮤텍스 획득 성공 블록을 종료합니다.
            else // 디버그 뮤텍스 획득에 실패한 경우입니다.
            { // 디버그 뮤텍스 획득 실패 블록을 시작합니다.
                ulErrCount_DebugMutexFail++; // 출력 누락(뮤텍스 실패) 횟수를 기록합니다.
            } // 디버그 뮤텍스 획득 실패 블록을 종료합니다.
        } // 센서 데이터 수신 성공 처리 블록을 종료합니다.

        vTaskDelay(pdMS_TO_TICKS(50U)); // 디버깅 태스크가 CPU를 과도하게 점유하지 않도록 50ms 지연합니다.
    } // 디버깅 태스크 무한 루프를 종료합니다.
} // 디버깅 태스크 함수를 종료합니다.

// --------------------------------------------------------------------------
// 펌웨어 설치 요청 (스테이징 패턴으로 전환)
//
// [기존 구조의 문제 — 셀프 플래싱]
//   예전에는 이 함수가 Bootloader_FlashNewFirmware() 를 호출해, 실행 중인
//   애플리케이션이 자기 자신이 들어 있는 플래시 영역을 지우고 다시 썼습니다.
//   이 구조는 원리적으로 복구가 불가능합니다.
//     1) 자기 코드를 지우는 순간부터 리셋되면 부팅할 코드 자체가 없습니다.
//        (전원 순단, 워치독 타임아웃, 브라운아웃 — 모두 현실에서 일어납니다)
//     2) 플래시 삭제/기록 중에는 같은 뱅크에서 명령을 페치할 수 없어,
//        플래싱 루틴 전체를 RAM 으로 옮기지 않으면 버스 폴트가 납니다.
//     3) 수신한 이미지가 손상되었어도 이미 원본을 지운 뒤라 되돌릴 수 없습니다.
//
// [바뀐 구조 — 스테이징]
//   실행 중인 코드는 절대 자기 영역을 건드리지 않습니다.
//     수신 → 스테이징 영역(섹터 5, 0x08020000)에 기록  ← 앱이 하는 일은 여기까지
//     검증 → CRC-32 + 헤더 + 벡터 테이블 유효성 확인
//     요청 → 백업 SRAM 의 BootCtrl 블록에 "설치 요청" 기록
//     리셋 → 섹터 0 의 부트로더가 검증·복사·점프를 수행
//   앱 영역을 지우는 주체가 부트로더(다른 섹터에서 실행)로 바뀌므로 위 3가지
//   문제가 모두 사라지고, 복사 도중 전원이 끊겨도 다음 부팅에서 부트로더가
//   시도 횟수를 보고 재설치하거나 복구 모드로 진입합니다.
//
// 따라서 이 함수는 더 이상 플래시를 쓰지 않습니다. 검증과 요청만 남습니다.
// -------------------------------------------------------------------------- 
static void prvRequestFirmwareInstall(void) // 펌웨어 설치 요청 함수를 정의합니다.
{ // 펌웨어 설치 요청 함수 본문을 시작합니다.
    IWDG_ReloadCounter(); // 요청 처리 진입 직전에 IWDG를 피드하여 타임아웃 여유를 확보합니다.

    // 스테이징 이미지를 마지막으로 한 번 더 검증합니다. 여기서 실패하면
// 설치 요청을 기록하지 않으므로 리셋도 일어나지 않고, 현재 펌웨어가 그대로
// 계속 동작합니다. (셀프 플래싱에서는 불가능했던 "안전한 취소"입니다) 
    if (Bootloader_VerifyStaged() != FW_IMAGE_OK) // 스테이징 영역 이미지의 CRC와 헤더를 검증합니다.
    { // 스테이징 이미지 검증 실패 처리 블록을 시작합니다.
        Bootloader_AbortStaging(); // 손상된 스테이징 이미지를 무효화합니다.
        xFirmwareUpdateActive = pdFALSE; // 워치독 태스크를 정상 감시 모드로 되돌립니다.
        IWDG_ReloadCounter(); // 정상 복귀 직전에 IWDG를 한 번 더 피드합니다.
        return; // 리셋하지 않고 현재 펌웨어로 계속 동작합니다.
    } // 스테이징 이미지 검증 실패 처리 블록을 종료합니다.

    IWDG_ReloadCounter(); // 리셋 직전에 한 번 더 피드하여 안전 마진을 확보합니다.

    // 백업 SRAM 에 설치 요청을 기록하고 NVIC_SystemReset() 을 호출합니다.
// 이 함수는 정상적으로 반환하지 않습니다. 리셋 후 섹터 0 의 부트로더가
// 스테이징 → 앱 영역 복사를 수행합니다. 
    Bootloader_RequestInstallAndReset(); // 설치를 요청하고 시스템을 리셋합니다.
} // 펌웨어 설치 요청 함수를 종료합니다.

void vTask_Firmware(void *pvParameters) // 펌웨어 업데이트 태스크 함수를 정의합니다.
{ // 펌웨어 업데이트 태스크 함수 본문을 시작합니다.
    (void)pvParameters; // 사용하지 않는 태스크 파라미터로 인한 경고를 방지합니다.
    FirmwareState_t xState; // 펌웨어 업데이트 요청 상태를 저장할 변수입니다.
    const TickType_t xCheckPeriod = pdMS_TO_TICKS(FIRMWARE_CHECK_PERIOD_MS); // 업데이트 요청 확인 주기 5초를 틱으로 변환합니다.

    Flash_Init(); // 플래시 메모리 제어 모듈을 초기화합니다.
    Bootloader_Init(); // 부트로더 인터페이스를 초기화합니다.

    for (;;) // 펌웨어 업데이트 태스크의 무한 루프를 시작합니다.
    { // 펌웨어 업데이트 태스크 루프 본문을 시작합니다.
        xState = Bootloader_CheckUpdateRequest(); // 부트로더 영역 또는 플래그에서 업데이트 요청을 확인합니다.
        if (xState == UPDATE_AVAILABLE) // 펌웨어 업데이트 요청이 존재하는지 확인합니다.
        { // 펌웨어 업데이트 처리 블록을 시작합니다.
            xFirmwareUpdateActive = pdTRUE; // 워치독 태스크가 무조건 피드하도록 업데이트 플래그를 설정합니다.

            // 업데이트 전에 안전 상태를 적용합니다. (액추에이터 뮤텍스로 보호) 
            if (xSemaphoreTake(xSemaphore_Actuator, portMAX_DELAY) == pdTRUE) // 액추에이터 뮤텍스 획득을 보장합니다.
            { // 안전 상태 적용 블록을 시작합니다.
                PWM_SetDuty(PWM_CHANNEL_THROTTLE, 0U); // 스로틀 출력을 0으로 안전화합니다.
                GPIO_Write(GPIO_PIN_FAN, 0U); // 팬 출력을 OFF로 안전화합니다.
                xSemaphoreGive(xSemaphore_Actuator); // 사용이 끝난 액추에이터 뮤텍스를 반환합니다.
            } // 안전 상태 적용 블록을 종료합니다.

            if (xTaskHandle_CAN_Rx != NULL) // CAN 태스크 핸들이 유효한지 확인합니다.
            { // CAN 태스크 정지 블록을 시작합니다.
                vTaskSuspend(xTaskHandle_CAN_Rx); // CAN 수신 태스크를 정지합니다.
            } // CAN 태스크 정지 블록을 종료합니다.
            if (xTaskHandle_Sensor != NULL) // 센서 태스크 핸들이 유효한지 확인합니다.
            { // 센서 태스크 정지 블록을 시작합니다.
                vTaskSuspend(xTaskHandle_Sensor); // 센서 수집 태스크를 정지합니다.
            } // 센서 태스크 정지 블록을 종료합니다.
            if (xTaskHandle_Actuator != NULL) // 액추에이터 태스크 핸들이 유효한지 확인합니다.
            { // 액추에이터 태스크 정지 블록을 시작합니다.
                vTaskSuspend(xTaskHandle_Actuator); // 액추에이터 제어 태스크를 정지합니다.
            } // 액추에이터 태스크 정지 블록을 종료합니다.
            if (xTaskHandle_Debug != NULL) // 디버깅 태스크 핸들이 유효한지 확인합니다.
            { // 디버깅 태스크 정지 블록을 시작합니다.
                vTaskSuspend(xTaskHandle_Debug); // 디버깅 태스크를 정지합니다.
            } // 디버깅 태스크 정지 블록을 종료합니다.
            // 워치독 태스크는 정지하지 않습니다: 업데이트 플래그를 보고 무조건 피드합니다. 

            // 스테이징 패턴에서는 이 태스크가 플래시를 쓰지 않으므로
// vTaskSuspendAll() 로 스케줄러를 멈출 필요가 없습니다.
// (오히려 멈추면 워치독 태스크까지 정지해 위험합니다. 예전 셀프 플래싱
//  구조에서 스케줄러를 멈춰야 했던 이유는 플래시 기록 중 컨텍스트 스위치를
//  막기 위해서였습니다.)
// 여기서 하는 일은 "검증 + 요청 기록 + 리셋" 뿐이며 수 밀리초면 끝납니다. 
            prvRequestFirmwareInstall(); // 스테이징 이미지를 검증하고 설치를 요청한 뒤 리셋합니다.

            // 여기에 도달했다는 것은 스테이징 이미지 검증에 실패해 설치를
// 취소했다는 뜻입니다. 현재 펌웨어로 계속 동작하며 다음 주기에 다시
// 확인합니다. (검증 성공 시에는 위에서 리셋되어 도달하지 않습니다) 
            ulErrCount_FirmwareStageInvalid++; // 스테이징 이미지 검증 실패 횟수를 기록합니다.

            // 정지시켰던 태스크들을 되살려 정상 운전으로 복귀합니다.
// 셀프 플래싱 구조에서는 "되돌아온다"는 개념 자체가 없었지만,
// 스테이징 구조에서는 설치를 취소하고 원래 상태로 돌아올 수 있습니다. 
            if (xTaskHandle_Debug != NULL) // 디버깅 태스크 핸들이 유효한지 확인합니다.
            { // 디버깅 태스크 재개 블록을 시작합니다.
                vTaskResume(xTaskHandle_Debug); // 디버깅 태스크를 재개합니다.
            } // 디버깅 태스크 재개 블록을 종료합니다.
            if (xTaskHandle_Actuator != NULL) // 액추에이터 태스크 핸들이 유효한지 확인합니다.
            { // 액추에이터 태스크 재개 블록을 시작합니다.
                vTaskResume(xTaskHandle_Actuator); // 액추에이터 제어 태스크를 재개합니다.
            } // 액추에이터 태스크 재개 블록을 종료합니다.
            if (xTaskHandle_Sensor != NULL) // 센서 태스크 핸들이 유효한지 확인합니다.
            { // 센서 태스크 재개 블록을 시작합니다.
                vTaskResume(xTaskHandle_Sensor); // 센서 수집 태스크를 재개합니다.
            } // 센서 태스크 재개 블록을 종료합니다.
            if (xTaskHandle_CAN_Rx != NULL) // CAN 태스크 핸들이 유효한지 확인합니다.
            { // CAN 태스크 재개 블록을 시작합니다.
                vTaskResume(xTaskHandle_CAN_Rx); // CAN 수신 태스크를 재개합니다.
            } // CAN 태스크 재개 블록을 종료합니다.

            xFirmwareUpdateActive = pdFALSE; // 워치독 태스크를 정상 하트비트 감시 모드로 되돌립니다.
        } // 펌웨어 업데이트 처리 블록을 종료합니다.

        vTaskDelay(xCheckPeriod); // 다음 업데이트 요청 확인까지 5초 동안 태스크를 지연합니다.
    } // 펌웨어 업데이트 태스크 무한 루프를 종료합니다.
} // 펌웨어 업데이트 태스크 함수를 종료합니다.

int main(void) // 프로그램 진입점 main 함수를 정의합니다.
{ // main 함수 본문을 시작합니다.
    BaseType_t xCreateResult = pdFAIL; // 태스크 생성 결과를 저장할 변수입니다.

    SystemClock_Config(); // 시스템 클록을 설정합니다.
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4); // STM32 NVIC 우선순위 그룹을 설정합니다.

    xQueue_ActuatorCmd = xQueueCreate(ACTUATOR_CMD_QUEUE_LEN, sizeof(ActuatorCmd_t)); // 액추에이터 명령 큐를 생성합니다.
    xQueue_Sensor_Data = xQueueCreate(SENSOR_DATA_QUEUE_LEN, sizeof(SensorData_t)); // 센서 데이터 큐를 생성합니다.
    xSemaphore_Actuator = xSemaphoreCreateMutex(); // 액추에이터 보호용 뮤텍스를 생성합니다.
    xMutex_Debug = xSemaphoreCreateMutex(); // 디버그 출력 보호용 뮤텍스를 생성합니다.

    configASSERT(xQueue_ActuatorCmd); // 액추에이터 명령 큐 생성 실패 시 시스템을 중단시킵니다.
    configASSERT(xQueue_Sensor_Data); // 센서 데이터 큐 생성 실패 시 시스템을 중단시킵니다.
    configASSERT(xSemaphore_Actuator); // 액추에이터 뮤텍스 생성 실패 시 시스템을 중단시킵니다.
    configASSERT(xMutex_Debug); // 디버그 뮤텍스 생성 실패 시 시스템을 중단시킵니다.

    // 태스크 생성 결과도 검사합니다.
// 주의: xTaskCreate() 호출을 configASSERT() 인자 안에 직접 넣으면,
// configASSERT가 비활성화된 빌드에서 태스크 생성 자체가 생략될 수 있습니다.
// 따라서 아래처럼 생성과 검사를 분리합니다. 
    xCreateResult = xTaskCreate(vTask_Watchdog, "Watchdog", TASK_WATCHDOG_STACK_SIZE, NULL, TASK_WATCHDOG_PRIORITY, &xTaskHandle_Watchdog); // 워치독 태스크를 생성합니다.
    configASSERT(xCreateResult == pdPASS); // 워치독 태스크 생성 실패 시 시스템을 중단시킵니다.
    xCreateResult = xTaskCreate(vTask_CAN_Rx, "CAN_Rx", TASK_CAN_STACK_SIZE, NULL, TASK_CAN_PRIORITY, &xTaskHandle_CAN_Rx); // CAN 수신 태스크를 생성합니다.
    configASSERT(xCreateResult == pdPASS); // CAN 태스크 생성 실패 시 시스템을 중단시킵니다.
    xCreateResult = xTaskCreate(vTask_Sensor, "Sensor", TASK_SENSOR_STACK_SIZE, NULL, TASK_SENSOR_PRIORITY, &xTaskHandle_Sensor); // 센서 태스크를 생성합니다.
    configASSERT(xCreateResult == pdPASS); // 센서 태스크 생성 실패 시 시스템을 중단시킵니다.
    xCreateResult = xTaskCreate(vTask_Actuator, "Actuator", TASK_ACTUATOR_STACK_SIZE, NULL, TASK_ACTUATOR_PRIORITY, &xTaskHandle_Actuator); // 액추에이터 태스크를 생성합니다.
    configASSERT(xCreateResult == pdPASS); // 액추에이터 태스크 생성 실패 시 시스템을 중단시킵니다.
    xCreateResult = xTaskCreate(vTask_Debug, "Debug", TASK_DEBUG_STACK_SIZE, NULL, TASK_DEBUG_PRIORITY, &xTaskHandle_Debug); // 디버그 태스크를 생성합니다.
    configASSERT(xCreateResult == pdPASS); // 디버그 태스크 생성 실패 시 시스템을 중단시킵니다.
    xCreateResult = xTaskCreate(vTask_Firmware, "Firmware", TASK_FIRMWARE_STACK_SIZE, NULL, TASK_FIRMWARE_PRIORITY, &xTaskHandle_Firmware); // 펌웨어 업데이트 태스크를 생성합니다.
    configASSERT(xCreateResult == pdPASS); // 펌웨어 태스크 생성 실패 시 시스템을 중단시킵니다.

    vTaskStartScheduler(); // FreeRTOS 스케줄러를 시작합니다.
    for (;;) // 스케줄러가 종료되는 비정상 상황을 대비한 무한 루프입니다.
    { // main 무한 루프 본문을 시작합니다.
    } // main 무한 루프 본문을 종료합니다.
    return 0; // main 함수 반환값입니다. 일반적으로 도달하지 않습니다.
} // main 함수를 종료합니다.
