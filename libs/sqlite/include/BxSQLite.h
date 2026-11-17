#ifndef BX_SQLITE_H
#define BX_SQLITE_H

#include <stdint.h>

#include <sqlite3.h>

#include <BxGC/BxGCAnnotations.h>

#ifdef __cplusplus
#define BX_SQLITE_API extern "C"
#else
#define BX_SQLITE_API extern
#endif

BX_SQLITE_API int32_t brixSQLitePrepare(
    sqlite3* database,
    const char* sql,
    int64_t length,
    sqlite3_stmt** statement,
    int32_t* hasMore
);

BX_SQLITE_API void brixSQLiteLock(sqlite3* database);
BX_SQLITE_API BRIX_WITHOUT_GC void brixSQLiteUnlock(sqlite3* database);

BX_SQLITE_API BRIX_WITHOUT_GC int32_t brixSQLiteBindNull(sqlite3_stmt* statement, int32_t index);
BX_SQLITE_API BRIX_WITHOUT_GC int32_t brixSQLiteBindInt64(
    sqlite3_stmt* statement,
    int32_t index,
    int64_t value
);
BX_SQLITE_API BRIX_WITHOUT_GC int32_t brixSQLiteBindDouble(
    sqlite3_stmt* statement,
    int32_t index,
    double value
);
BX_SQLITE_API BRIX_WITHOUT_GC int32_t brixSQLiteBindText(
    sqlite3_stmt* statement,
    int32_t index,
    const char* text,
    int64_t length
);
BX_SQLITE_API BRIX_WITHOUT_GC int32_t brixSQLiteBindBlob(
    sqlite3_stmt* statement,
    int32_t index,
    const void* bytes,
    int64_t length
);

BX_SQLITE_API BRIX_WITHOUT_GC int32_t brixSQLiteColumnCount(sqlite3_stmt* statement);
BX_SQLITE_API BRIX_WITHOUT_GC int32_t brixSQLiteReadType(sqlite3_stmt* statement, int64_t index);
BX_SQLITE_API BRIX_WITHOUT_GC int64_t brixSQLiteReadInt64(
    sqlite3_stmt* statement,
    int64_t index,
    int32_t* type
);
BX_SQLITE_API BRIX_WITHOUT_GC double brixSQLiteReadDouble(
    sqlite3_stmt* statement,
    int64_t index,
    int32_t* type
);
BX_SQLITE_API BRIX_WITHOUT_GC const char* brixSQLiteReadText(
    sqlite3_stmt* statement,
    int64_t index,
    int32_t* type,
    int64_t* length
);
BX_SQLITE_API BRIX_WITHOUT_GC const void* brixSQLiteReadBlob(
    sqlite3_stmt* statement,
    int64_t index,
    int32_t* type,
    int64_t* length
);

#endif
