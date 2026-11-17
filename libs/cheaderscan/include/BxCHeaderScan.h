#ifndef BX_CHEADERSCAN_H
#define BX_CHEADERSCAN_H

#include <stdint.h>

#ifdef __cplusplus
#define BX_CHEADERSCAN_API extern "C"
#else
#define BX_CHEADERSCAN_API extern
#endif

BX_CHEADERSCAN_API const char* brixCHeaderScanVersion(void);
BX_CHEADERSCAN_API void brixCHeaderScanUseLibclang(const char* path);
BX_CHEADERSCAN_API char* brixCHeaderScan(
    const char* header,
    const char* language,
    const char* arguments,
    int32_t* status
);
BX_CHEADERSCAN_API void brixCHeaderScanFree(char* records);

#endif
