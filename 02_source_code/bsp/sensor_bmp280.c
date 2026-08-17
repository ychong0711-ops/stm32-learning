// sensor_bmp280.c — BMP280 온도 센서 구현 (I2C, STM32F4 HAL)입니다.
// I2C1(PB6/PB7)로 통신하며, 데이터시트 보상 공식으로 온도를 계산합니다.
#include "sensor_bmp280.h"  // BMP280 센서 헤더를 포함합니다.

#include "stm32f4xx_hal.h"  // STM32F4 HAL 드라이버 헤더를 포함합니다.

#define BMP280_REG_CHIP_ID     0xD0U  // 칩 ID 레지스터 주소를 정의합니다.
#define BMP280_REG_RESET       0xE0U  // 리셋 레지스터 주소를 정의합니다.
#define BMP280_REG_STATUS      0xF3U  // 상태 레지스터 주소를 정의합니다.
#define BMP280_REG_CTRL_MEAS   0xF4U  // 측정 제어 레지스터 주소를 정의합니다.
#define BMP280_REG_CONFIG      0xF5U  // 설정 레지스터 주소를 정의합니다.
#define BMP280_REG_TEMP_MSB    0xFAU  // 온도 MSB 레지스터 주소를 정의합니다.
#define BMP280_REG_TEMP_LSB    0xFBU  // 온도 LSB 레지스터 주소를 정의합니다.
#define BMP280_REG_TEMP_XLSB   0xFCU  // 온도 XLSB 레지스터 주소를 정의합니다.

#define BMP280_CHIP_ID_VALUE   0x58U  // BMP280 의 정상 칩 ID 값을 정의합니다.
#define BMP280_RESET_CMD       0xB6U  // 소프트 리셋 명령 값을 정의합니다.

typedef struct  // BMP280 보정 계수 구조체 정의를 시작합니다.
{  // 보정 계수 멤버 선언을 시작합니다.
    uint16_t dig_T1;  // 온도 보정 계수 T1 을 저장합니다.
    int16_t  dig_T2;  // 온도 보정 계수 T2 를 저장합니다.
    int16_t  dig_T3;  // 온도 보정 계수 T3 를 저장합니다.
} Bmp280Calib_t;  // 보정 계수 구조체 타입 이름을 Bmp280Calib_t 로 정의합니다.

static I2C_HandleTypeDef s_hi2c1;  // I2C1 의 HAL 핸들 구조체입니다.
static Bmp280Calib_t s_calib;  // 로드한 보정 계수를 저장하는 변수입니다.
static int32_t s_lTFine = 0;  // 온도 보정 중간값(t_fine)을 저장하는 변수입니다.

static HAL_StatusTypeDef prvReadReg(uint8_t ucReg, uint8_t *pData, uint16_t usLen)  // I2C 레지스터 읽기 내부 함수를 정의합니다.
{  // 레지스터 읽기 함수 본문을 시작합니다.
    return HAL_I2C_Mem_Read(&s_hi2c1, (uint16_t)(BMP280_I2C_ADDR << 1),  // I2C 메모리 읽기를 수행합니다.
                            ucReg, I2C_MEMADD_SIZE_8BIT, pData, usLen, 100U);  // 레지스터 주소, 8bit 크기, 타임아웃 100ms 를 지정합니다.
}  // 레지스터 읽기 함수를 종료합니다.

static HAL_StatusTypeDef prvWriteReg(uint8_t ucReg, uint8_t ucData)  // I2C 레지스터 쓰기 내부 함수를 정의합니다.
{  // 레지스터 쓰기 함수 본문을 시작합니다.
    return HAL_I2C_Mem_Write(&s_hi2c1, (uint16_t)(BMP280_I2C_ADDR << 1),  // I2C 메모리 쓰기를 수행합니다.
                             ucReg, I2C_MEMADD_SIZE_8BIT, &ucData, 1U, 100U);  // 레지스터 주소와 1바이트 데이터를 지정합니다.
}  // 레지스터 쓰기 함수를 종료합니다.

static void prvI2cInit(void)  // I2C1 초기화 내부 함수를 정의합니다.
{  // I2C 초기화 함수 본문을 시작합니다.
    GPIO_InitTypeDef GPIO_InitStruct = {0};  // GPIO 초기화 구조체를 0으로 선언합니다.

    __HAL_RCC_GPIOB_CLK_ENABLE();  // GPIOB 클록을 활성화합니다.
    __HAL_RCC_I2C1_CLK_ENABLE();  // I2C1 클록을 활성화합니다.

    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;  // PB6(SCL), PB7(SDA) 핀을 선택합니다.
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;  // 대체 기능 오픈드레인 모드로 설정합니다. (I2C 요구)
    GPIO_InitStruct.Pull = GPIO_PULLUP;  // 내부 풀업을 활성화합니다.
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;  // 핀 속도를 최고속으로 설정합니다.
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;  // 대체 기능을 AF4(I2C1)로 설정합니다.
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);  // 설정을 GPIOB 에 적용합니다.

    s_hi2c1.Instance = I2C1;  // HAL 핸들에 I2C1 인스턴스를 연결합니다.
    s_hi2c1.Init.ClockSpeed = 100000U;  // I2C 클록을 100kHz 로 설정합니다.
    s_hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;  // 듀티 사이클을 2:1 로 설정합니다.
    s_hi2c1.Init.OwnAddress1 = 0;  // 자체 주소1 을 0으로 설정합니다.
    s_hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;  // 7bit 주소 모드로 설정합니다.
    s_hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;  // 듀얼 주소 모드를 비활성화합니다.
    s_hi2c1.Init.OwnAddress2 = 0;  // 자체 주소2 를 0으로 설정합니다.
    s_hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;  // 일반 호출 모드를 비활성화합니다.
    s_hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;  // 클록 스트레칭을 허용합니다.
    (void)HAL_I2C_Init(&s_hi2c1);  // I2C1 을 초기화합니다.
}  // I2C 초기화 함수를 종료합니다.

void BMP280_Init(void)  // BMP280 초기화 함수를 정의합니다.
{  // BMP280 초기화 함수 본문을 시작합니다.
    uint8_t ucReg[6];  // 보정 계수를 읽을 6바이트 버퍼를 선언합니다.
    uint8_t ucChipId = 0;  // 칩 ID 를 저장할 변수를 선언합니다.

    prvI2cInit();  // I2C1 을 초기화합니다.

    (void)prvWriteReg(BMP280_REG_RESET, BMP280_RESET_CMD);  // 소프트 리셋 명령을 전송합니다.
    HAL_Delay(10);  // 리셋 안정화를 위해 10ms 대기합니다.

    if (prvReadReg(BMP280_REG_CHIP_ID, &ucChipId, 1U) != HAL_OK)  // 칩 ID 를 읽어옵니다.
    {  // I2C 통신 실패 처리 블록을 시작합니다.
        return;  // 통신에 실패하면 초기화를 중단합니다.
    }  // I2C 통신 실패 처리 블록을 종료합니다.
    if (ucChipId != BMP280_CHIP_ID_VALUE)  // 읽은 칩 ID 가 기대값과 일치하는지 확인합니다.
    {  // 칩 ID 불일치 처리 블록을 시작합니다.
        return;  // 다른 칩이면 초기화를 중단합니다.
    }  // 칩 ID 불일치 처리 블록을 종료합니다.

    if (prvReadReg(0x88U, ucReg, 6U) == HAL_OK)  // 0x88 부터 보정 계수 6바이트를 읽습니다.
    {  // 보정 계수 로드 블록을 시작합니다.
        s_calib.dig_T1 = (uint16_t)(ucReg[0] | ((uint16_t)ucReg[1] << 8));  // T1 계수를 조립해 저장합니다.
        s_calib.dig_T2 = (int16_t)(ucReg[2] | ((uint16_t)ucReg[3] << 8));  // T2 계수를 조립해 저장합니다.
        s_calib.dig_T3 = (int16_t)(ucReg[4] | ((uint16_t)ucReg[5] << 8));  // T3 계수를 조립해 저장합니다.
    }  // 보정 계수 로드 블록을 종료합니다.

    (void)prvWriteReg(BMP280_REG_CTRL_MEAS, 0x27U);  // 온도 오버샘플링 x1, 노멀 모드로 측정 설정을 기록합니다.
    HAL_Delay(10);  // 설정 반영을 위해 10ms 대기합니다.
}  // BMP280 초기화 함수를 종료합니다.

float BMP280_ReadTemperature(void)  // 온도 읽기 함수를 정의합니다.
{  // 온도 읽기 함수 본문을 시작합니다.
    uint8_t ucData[3];  // 온도 원시값 3바이트를 저장할 버퍼입니다.
    uint32_t ulAdcT;  // 20bit 원시 온도 값을 저장할 변수입니다.

    if (prvReadReg(BMP280_REG_TEMP_MSB, ucData, 3U) != HAL_OK)  // 온도 원시값 3바이트를 읽습니다.
    {  // 읽기 실패 처리 블록을 시작합니다.
        return 0.0f;  // 실패 시 0.0°C 를 반환합니다.
    }  // 읽기 실패 처리 블록을 종료합니다.

    ulAdcT = ((uint32_t)ucData[0] << 12) |  // MSB 를 상위 12bit 로 이동합니다.
             ((uint32_t)ucData[1] << 4)  |  // LSB 를 중간 8bit 로 이동합니다.
             ((uint32_t)ucData[2] >> 4);    // XLSB 의 상위 4bit 를 결합하여 20bit 값을 만듭니다.

    int32_t var1 = ((((int32_t)ulAdcT >> 3) - ((int32_t)s_calib.dig_T1 << 1)) *  // 보상 공식 var1 의 첫 번째 항을 계산합니다.
                    (int32_t)s_calib.dig_T2) >> 11;  // T2 를 곱하고 11bit 오른쪽 시프트하여 var1 을 완성합니다.
    int32_t lVar2T = (((int32_t)ulAdcT >> 4) - (int32_t)s_calib.dig_T1);  // (adc_T>>4 − dig_T1) 을 계산해 임시 변수에 저장합니다.
    int32_t var2 = (((lVar2T * lVar2T) >> 12) * (int32_t)s_calib.dig_T3) >> 14;  // 데이터시트 공식대로 (X·X)를 계산해 var2 를 완성합니다.
    s_lTFine = var1 + var2;  // t_fine(온도 보정 중간값)을 계산해 저장합니다.

    int32_t lTempX100 = (s_lTFine * 5 + 128) >> 8;  // 0.01°C 단위 온도를 계산합니다.
    return (float)lTempX100 / 100.0f;  // 섭씨(°C) float 값으로 변환해 반환합니다.
}  // 온도 읽기 함수를 종료합니다.
