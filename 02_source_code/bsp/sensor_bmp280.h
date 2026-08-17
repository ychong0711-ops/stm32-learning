// sensor_bmp280.h — BMP280 온도/압력 센서 BSP 헤더 (I2C, STM32F4 HAL 기반)입니다.
// I2C1 (PB6/PB7) 을 통해 센서와 통신합니다.
#ifndef SENSOR_BMP280_H  // SENSOR_BMP280_H 가 아직 정의되지 않았는지 확인합니다.
#define SENSOR_BMP280_H  // SENSOR_BMP280_H 를 정의하여 중복 포함을 방지합니다.

#include <stdint.h>  // uint32_t 등 고정 폭 정수 타입을 사용하기 위해 포함합니다.

#define BMP280_I2C_ADDR   0x76U  // BMP280 의 I2C 7bit 주소를 정의합니다. (SDO 연결에 따라 0x76 또는 0x77)

void BMP280_Init(void);  // BMP280 를 초기화하는 함수 프로토타입입니다. (I2C + 보정 계수 로드)
float BMP280_ReadTemperature(void);  // 온도를 섭씨(°C) float 로 읽어 반환하는 함수 프로토타입입니다.

#endif /* SENSOR_BMP280_H */  // SENSOR_BMP280_H 조건부 컴파일 블록을 종료합니다.
