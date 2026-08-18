// fw_image.c — 펌웨어 이미지 헤더 검증과 CRC-32 계산 구현입니다.
// 표준 CRC-32 (IEEE 802.3, 다항식 0xEDB88320 반사형, 초기값 0xFFFFFFFF, 최종 반전)를 사용합니다.
// 테이블 없이 비트 단위로 계산하여 부트로더의 코드 크기를 최소화합니다.
#include "fw_image.h"  // 이미지 헤더 구조체와 프로토타입을 포함합니다.
#include "flash_map.h"  // 앱 영역 주소/크기 상수를 사용하기 위해 포함합니다.

#define CRC32_POLY_REFLECTED 0xEDB88320U  // 반사형 CRC-32 생성 다항식입니다.
#define CRC32_INIT           0xFFFFFFFFU  // CRC-32 계산의 초기값입니다.

uint32_t FwImage_Crc32Update(uint32_t ulCrc, const void *pvData, uint32_t ulLength)  // CRC-32 누적 갱신 함수를 정의합니다.
{  // CRC 누적 갱신 함수 본문을 시작합니다.
    const uint8_t *pucData = (const uint8_t *)pvData;  // 바이트 단위로 접근하기 위해 포인터를 변환합니다.

    for (uint32_t i = 0U; i < ulLength; i++)  // 입력 바이트 수만큼 반복합니다.
    {  // 바이트 처리 반복문 본문을 시작합니다.
        ulCrc ^= (uint32_t)pucData[i];  // 현재 바이트를 CRC 하위 8비트에 배타적 논리합합니다.
        for (uint8_t b = 0U; b < 8U; b++)  // 한 바이트의 8비트를 순회합니다.
        {  // 비트 처리 반복문 본문을 시작합니다.
            uint32_t ulMask = (uint32_t)(-(int32_t)(ulCrc & 1U));  // 최하위 비트가 1이면 0xFFFFFFFF, 0이면 0을 만듭니다.
            ulCrc = (ulCrc >> 1) ^ (CRC32_POLY_REFLECTED & ulMask);  // 분기 없이 다항식을 조건부로 적용합니다.
        }  // 비트 처리 반복문을 종료합니다.
    }  // 바이트 처리 반복문을 종료합니다.

    return ulCrc;  // 갱신된 중간 CRC 값을 반환합니다.
}  // CRC 누적 갱신 함수를 종료합니다.

uint32_t FwImage_Crc32Finalize(uint32_t ulCrc)  // 누적 CRC 를 최종 값으로 변환하는 함수를 정의합니다.
{  // CRC 최종화 함수 본문을 시작합니다.
    return ulCrc ^ 0xFFFFFFFFU;  // 표준 CRC-32 규칙에 따라 최종 반전을 수행합니다.
}  // CRC 최종화 함수를 종료합니다.

uint32_t FwImage_Crc32(const void *pvData, uint32_t ulLength)  // 한 번에 CRC-32 를 계산하는 함수를 정의합니다.
{  // CRC 일괄 계산 함수 본문을 시작합니다.
    uint32_t ulCrc = FwImage_Crc32Update(CRC32_INIT, pvData, ulLength);  // 초기값에서 시작해 전체 버퍼를 누적합니다.
    return FwImage_Crc32Finalize(ulCrc);  // 최종 반전을 적용한 CRC 를 반환합니다.
}  // CRC 일괄 계산 함수를 종료합니다.

FwImageStatus_t FwImage_CheckHeader(const FwImageHeader_t *pxHeader)  // 헤더 유효성 검사 함수를 정의합니다.
{  // 헤더 검사 함수 본문을 시작합니다.
    uint32_t ulHdrCrc;  // 계산된 헤더 CRC 를 담을 변수입니다.

    if (pxHeader == 0)  // 헤더 포인터가 NULL 인지 확인합니다.
    {  // NULL 처리 블록을 시작합니다.
        return FW_IMAGE_ERR_MAGIC;  // 매직 오류로 간주하여 반환합니다.
    }  // NULL 처리 블록을 종료합니다.

    if (pxHeader->ulMagic != FW_IMAGE_MAGIC)  // 매직 값이 기대와 다른지 확인합니다.
    {  // 매직 불일치 처리 블록을 시작합니다.
        return FW_IMAGE_ERR_MAGIC;  // 매직 오류를 반환합니다.
    }  // 매직 불일치 처리 블록을 종료합니다.

    if ((pxHeader->ulHeaderVersion >> 16) != (FW_IMAGE_HEADER_VERSION >> 16))  // 상위 버전(메이저)이 다른지 확인합니다.
    {  // 버전 불일치 처리 블록을 시작합니다.
        return FW_IMAGE_ERR_VERSION;  // 버전 오류를 반환합니다.
    }  // 버전 불일치 처리 블록을 종료합니다.

    if ((pxHeader->ulImageSize == 0U) ||  // 이미지 크기가 0 인지 확인합니다.
        (pxHeader->ulImageSize > STAGE_IMAGE_MAX_SIZE) ||  // 이미지 크기가 허용 최대치를 넘는지 확인합니다.
        ((pxHeader->ulImageSize & 0x3U) != 0U))  // 이미지 크기가 4의 배수가 아닌지 확인합니다.
    {  // 크기 오류 처리 블록을 시작합니다.
        return FW_IMAGE_ERR_SIZE;  // 크기 오류를 반환합니다.
    }  // 크기 오류 처리 블록을 종료합니다.

    if (pxHeader->ulLoadAddr != APP_REGION_ADDR)  // 실행 주소가 앱 영역 시작 주소와 다른지 확인합니다.
    {  // 주소 오류 처리 블록을 시작합니다.
        return FW_IMAGE_ERR_ADDR;  // 주소 오류를 반환합니다.
    }  // 주소 오류 처리 블록을 종료합니다.

    ulHdrCrc = FwImage_Crc32(pxHeader, FW_IMAGE_HEADER_SIZE - 4U);  // 마지막 CRC 필드를 제외한 28바이트의 CRC 를 계산합니다.
    if (ulHdrCrc != pxHeader->ulHeaderCrc32)  // 계산 결과가 헤더에 기록된 값과 다른지 확인합니다.
    {  // 헤더 CRC 불일치 처리 블록을 시작합니다.
        return FW_IMAGE_ERR_HDR_CRC;  // 헤더 CRC 오류를 반환합니다.
    }  // 헤더 CRC 불일치 처리 블록을 종료합니다.

    return FW_IMAGE_OK;  // 헤더가 모두 정상임을 반환합니다.
}  // 헤더 검사 함수를 종료합니다.

int FwImage_IsVectorTableSane(const void *pvImage, uint32_t ulLoadAddr, uint32_t ulRegionSize)  // 벡터 테이블 정상 여부 확인 함수를 정의합니다.
{  // 벡터 테이블 검사 함수 본문을 시작합니다.
    const uint32_t *pulVector = (const uint32_t *)pvImage;  // 이미지 선두를 벡터 테이블로 해석합니다.
    uint32_t ulInitialSp;  // 초기 스택 포인터 값을 담을 변수입니다.
    uint32_t ulResetPc;  // 리셋 핸들러 주소를 담을 변수입니다.

    if (pvImage == 0)  // 이미지 포인터가 NULL 인지 확인합니다.
    {  // NULL 처리 블록을 시작합니다.
        return 0;  // 비정상(0)을 반환합니다.
    }  // NULL 처리 블록을 종료합니다.

    ulInitialSp = pulVector[0];  // 벡터 테이블 0번 워드에서 초기 스택 포인터를 읽습니다.
    ulResetPc = pulVector[1];  // 벡터 테이블 1번 워드에서 리셋 핸들러 주소를 읽습니다.

    // 초기 스택 포인터는 SRAM 영역(0x20000000 ~ 0x20020000, 128KB)을 가리켜야 합니다.
    if ((ulInitialSp < 0x20000000U) || (ulInitialSp > 0x20020000U))  // 스택 포인터가 SRAM 범위를 벗어나는지 확인합니다.
    {  // 스택 포인터 이상 처리 블록을 시작합니다.
        return 0;  // 비정상(0)을 반환합니다.
    }  // 스택 포인터 이상 처리 블록을 종료합니다.

    // 리셋 핸들러는 자기 영역 안을 가리키고, Thumb 모드 비트(bit0=1)가 세워져 있어야 합니다.
    if ((ulResetPc & 1U) == 0U)  // Thumb 비트가 꺼져 있는지 확인합니다.
    {  // Thumb 비트 이상 처리 블록을 시작합니다.
        return 0;  // 비정상(0)을 반환합니다.
    }  // Thumb 비트 이상 처리 블록을 종료합니다.

    if ((ulResetPc < ulLoadAddr) || (ulResetPc >= (ulLoadAddr + ulRegionSize)))  // 리셋 핸들러가 영역 밖을 가리키는지 확인합니다.
    {  // 리셋 핸들러 범위 이상 처리 블록을 시작합니다.
        return 0;  // 비정상(0)을 반환합니다.
    }  // 리셋 핸들러 범위 이상 처리 블록을 종료합니다.

    return 1;  // 벡터 테이블이 정상임(1)을 반환합니다.
}  // 벡터 테이블 검사 함수를 종료합니다.

FwImageStatus_t FwImage_Verify(const FwImageHeader_t *pxHeader, const void *pvImage)  // 헤더와 본문을 모두 검증하는 함수를 정의합니다.
{  // 전체 검증 함수 본문을 시작합니다.
    FwImageStatus_t xStatus = FwImage_CheckHeader(pxHeader);  // 먼저 헤더 자체의 유효성을 확인합니다.

    if (xStatus != FW_IMAGE_OK)  // 헤더 검사에서 이미 실패했는지 확인합니다.
    {  // 헤더 실패 처리 블록을 시작합니다.
        return xStatus;  // 헤더 검사 결과를 그대로 반환합니다.
    }  // 헤더 실패 처리 블록을 종료합니다.

    // 본문 포인터를 검사합니다. FwImage_CheckHeader() 는 헤더만 보므로 본문이
    // NULL 인 경우를 걸러 주지 않습니다. 아래 FwImage_Crc32() 는 길이가 0 이
    // 아닌 이상(헤더 검사가 크기 0 을 이미 배제했습니다) 반드시 역참조하므로,
    // 여기서 막지 않으면 NULL 역참조가 됩니다.
    if (pvImage == 0)  // 본문 포인터가 NULL 인지 확인합니다.
    {  // NULL 처리 블록을 시작합니다.
        return FW_IMAGE_ERR_ADDR;  // 주소 오류로 반환합니다.
    }  // NULL 처리 블록을 종료합니다.

    if (FwImage_Crc32(pvImage, pxHeader->ulImageSize) != pxHeader->ulImageCrc32)  // 본문 CRC 가 헤더 값과 다른지 확인합니다.
    {  // 본문 CRC 불일치 처리 블록을 시작합니다.
        return FW_IMAGE_ERR_IMG_CRC;  // 본문 CRC 오류를 반환합니다.
    }  // 본문 CRC 불일치 처리 블록을 종료합니다.

    if (FwImage_IsVectorTableSane(pvImage, pxHeader->ulLoadAddr, APP_REGION_SIZE) == 0)  // 벡터 테이블이 비정상인지 확인합니다.
    {  // 벡터 테이블 이상 처리 블록을 시작합니다.
        return FW_IMAGE_ERR_VECTOR;  // 벡터 테이블 오류를 반환합니다.
    }  // 벡터 테이블 이상 처리 블록을 종료합니다.

    return FW_IMAGE_OK;  // 이미지 전체가 유효함을 반환합니다.
}  // 전체 검증 함수를 종료합니다.
