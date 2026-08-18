// fw_image.h — 펌웨어 이미지 헤더와 무결성 검증(CRC-32) 인터페이스입니다.
// 부트로더와 애플리케이션이 동일한 규칙으로 이미지를 해석하도록 공유합니다.
// 호스트 도구 tools/make_image.py 가 같은 알고리즘으로 헤더를 생성합니다.
#ifndef FW_IMAGE_H  // FW_IMAGE_H 가 아직 정의되지 않았는지 확인합니다.
#define FW_IMAGE_H  // FW_IMAGE_H 를 정의하여 중복 포함을 방지합니다.

#include <stdint.h>  // uint32_t 등 고정 폭 정수 타입을 사용하기 위해 포함합니다.

#define FW_IMAGE_MAGIC          0x4D465733U  // 이미지 헤더 매직 값('3WFM' 리틀엔디언 표기)입니다.
#define FW_IMAGE_HEADER_VERSION 0x00010000U  // 헤더 포맷 버전(1.0)입니다.
#define FW_IMAGE_HEADER_SIZE    32U          // 헤더 구조체의 바이트 크기입니다. (고정 32바이트)

typedef struct  // 펌웨어 이미지 헤더 구조체 정의를 시작합니다.
{  // 이미지 헤더 멤버 선언을 시작합니다.
    uint32_t ulMagic;         // 이미지 헤더 매직 값(FW_IMAGE_MAGIC)을 저장합니다.
    uint32_t ulHeaderVersion; // 헤더 포맷 버전을 저장합니다.
    uint32_t ulImageSize;     // 이미지 본문의 바이트 크기를 저장합니다. (4의 배수)
    uint32_t ulImageCrc32;    // 이미지 본문 전체의 CRC-32 값을 저장합니다.
    uint32_t ulFwVersion;     // 펌웨어 버전(예: 0x00010203 = v1.2.3)을 저장합니다.
    uint32_t ulLoadAddr;      // 이미지가 실행될 주소를 저장합니다. (APP_REGION_ADDR 이어야 함)
    uint32_t ulReserved;      // 향후 확장을 위한 예약 필드입니다. (0 으로 채움)
    uint32_t ulHeaderCrc32;   // 앞 28바이트에 대한 CRC-32 값을 저장합니다.
} FwImageHeader_t;  // 이미지 헤더 타입 이름을 FwImageHeader_t 로 정의합니다.

typedef enum  // 이미지 검증 결과 열거형 정의를 시작합니다.
{  // 검증 결과 값 선언을 시작합니다.
    FW_IMAGE_OK = 0,        // 헤더와 본문이 모두 유효함을 의미합니다.
    FW_IMAGE_ERR_MAGIC,     // 매직 값이 일치하지 않음을 의미합니다.
    FW_IMAGE_ERR_VERSION,   // 헤더 포맷 버전이 지원 범위를 벗어남을 의미합니다.
    FW_IMAGE_ERR_SIZE,      // 이미지 크기가 0이거나 허용 범위를 초과함을 의미합니다.
    FW_IMAGE_ERR_ADDR,      // 실행 주소가 앱 영역 시작 주소와 다름을 의미합니다.
    FW_IMAGE_ERR_HDR_CRC,   // 헤더 CRC 가 일치하지 않음을 의미합니다.
    FW_IMAGE_ERR_IMG_CRC,   // 본문 CRC 가 일치하지 않음을 의미합니다.
    FW_IMAGE_ERR_VECTOR     // 벡터 테이블(초기 SP/PC)이 비정상임을 의미합니다.
} FwImageStatus_t;  // 검증 결과 타입 이름을 FwImageStatus_t 로 정의합니다.

uint32_t FwImage_Crc32(const void *pvData, uint32_t ulLength);  // 버퍼의 CRC-32 를 계산하는 함수 프로토타입입니다.
uint32_t FwImage_Crc32Update(uint32_t ulCrc, const void *pvData, uint32_t ulLength);  // CRC-32 를 누적 갱신하는 함수 프로토타입입니다.
uint32_t FwImage_Crc32Finalize(uint32_t ulCrc);  // 누적된 CRC-32 를 최종 값으로 변환하는 함수 프로토타입입니다.

FwImageStatus_t FwImage_CheckHeader(const FwImageHeader_t *pxHeader);  // 헤더 자체의 유효성을 검사하는 함수 프로토타입입니다.
FwImageStatus_t FwImage_Verify(const FwImageHeader_t *pxHeader, const void *pvImage);  // 헤더 + 본문 CRC 까지 검증하는 함수 프로토타입입니다.
int FwImage_IsVectorTableSane(const void *pvImage, uint32_t ulLoadAddr, uint32_t ulRegionSize);  // 벡터 테이블 정상 여부를 확인하는 함수 프로토타입입니다.

#endif /* FW_IMAGE_H */  // FW_IMAGE_H 조건부 컴파일 블록을 종료합니다.
