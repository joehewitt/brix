#ifndef YU_SQLITE_H
#define YU_SQLITE_H

#include <stdint.h>

#include <sqlite3.h>

#include <YuGC/YuGCAnnotations.h>

#ifdef __cplusplus
#define YU_SQLITE_API extern "C"
#else
#define YU_SQLITE_API extern
#endif

YU_SQLITE_API int32_t yumiSQLitePrepare(
    sqlite3* database,
    const char* sql,
    int64_t length,
    sqlite3_stmt** statement,
    int32_t* hasMore
);

YU_SQLITE_API void yumiSQLiteLock(sqlite3* database);
YU_SQLITE_API YUMI_WITHOUT_GC void yumiSQLiteUnlock(sqlite3* database);

YU_SQLITE_API YUMI_WITHOUT_GC int32_t yumiSQLiteBindNull(sqlite3_stmt* statement, int32_t index);
YU_SQLITE_API YUMI_WITHOUT_GC int32_t yumiSQLiteBindInt64(
    sqlite3_stmt* statement,
    int32_t index,
    int64_t value
);
YU_SQLITE_API YUMI_WITHOUT_GC int32_t yumiSQLiteBindDouble(
    sqlite3_stmt* statement,
    int32_t index,
    double value
);
YU_SQLITE_API YUMI_WITHOUT_GC int32_t yumiSQLiteBindText(
    sqlite3_stmt* statement,
    int32_t index,
    const char* text,
    int64_t length
);
YU_SQLITE_API YUMI_WITHOUT_GC int32_t yumiSQLiteBindBlob(
    sqlite3_stmt* statement,
    int32_t index,
    const void* bytes,
    int64_t length
);

YU_SQLITE_API YUMI_WITHOUT_GC int32_t yumiSQLiteColumnCount(sqlite3_stmt* statement);
YU_SQLITE_API YUMI_WITHOUT_GC int32_t yumiSQLiteReadType(sqlite3_stmt* statement, int64_t index);
YU_SQLITE_API YUMI_WITHOUT_GC int64_t yumiSQLiteReadInt64(
    sqlite3_stmt* statement,
    int64_t index,
    int32_t* type
);
YU_SQLITE_API YUMI_WITHOUT_GC double yumiSQLiteReadDouble(
    sqlite3_stmt* statement,
    int64_t index,
    int32_t* type
);
YU_SQLITE_API YUMI_WITHOUT_GC const char* yumiSQLiteReadText(
    sqlite3_stmt* statement,
    int64_t index,
    int32_t* type,
    int64_t* length
);
YU_SQLITE_API YUMI_WITHOUT_GC const void* yumiSQLiteReadBlob(
    sqlite3_stmt* statement,
    int64_t index,
    int32_t* type,
    int64_t* length
);

#endif
