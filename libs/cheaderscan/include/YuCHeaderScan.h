#ifndef YU_CHEADERSCAN_H
#define YU_CHEADERSCAN_H

#include <stdint.h>

#ifdef __cplusplus
#define YU_CHEADERSCAN_API extern "C"
#else
#define YU_CHEADERSCAN_API extern
#endif

YU_CHEADERSCAN_API const char* yumiCHeaderScanVersion(void);
YU_CHEADERSCAN_API void yumiCHeaderScanUseLibclang(const char* path);
YU_CHEADERSCAN_API char* yumiCHeaderScan(
    const char* header,
    const char* language,
    const char* arguments,
    int32_t* status
);
YU_CHEADERSCAN_API void yumiCHeaderScanFree(char* records);

#endif
