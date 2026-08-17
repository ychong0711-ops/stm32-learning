/* bmp280_calc.c — 데모용: 프로젝트의 실제 함수를 독립 추출 (정적 분석 대상)
 * 센서 보정 계수와 20bit 원시값으로부터 보상 온도(t_fine)를 계산하는
 * 데이터시트 공식입니다. 순수 정수 연산이라 헤더 의존성이 없습니다.
 */
#include <stdint.h>

typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
} Bmp280Calib_t;

/* BMP280 데이터시트 보상 공식 (우리 프로젝트 bsp/sensor_bmp280.c 에서 발췌) */
int32_t bmp280_compensate(uint32_t ulAdcT, const Bmp280Calib_t *pCal)
{
    int32_t lVar2T = (((int32_t)ulAdcT >> 4) - (int32_t)pCal->dig_T1);          /* (adc>>4) - T1 */
    int32_t var2   = (((lVar2T * lVar2T) >> 12) * (int32_t)pCal->dig_T3) >> 14; /* ((X*X)>>12)*T3 >>14 */
    int32_t var1   = (((((int32_t)ulAdcT >> 3) - ((int32_t)pCal->dig_T1 << 1)) *
                       (int32_t)pCal->dig_T2) >> 11);                           /* t_fine 의 var1 */
    return var1 + var2;
}

/* CAN 페이로드 → 16bit 리틀엔디언 조립 (bsp_can.c 의 스로틀 파싱 로직) */
uint16_t can_assemble_le(const uint8_t data[2])
{
    return (uint16_t)(((uint16_t)data[1] << 8) | (uint16_t)data[0]);
}
