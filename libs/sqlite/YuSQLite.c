#include "include/YuSQLite.h"

#include <stddef.h>

int32_t yumiSQLitePrepare(
    sqlite3* database,
    const char* sql,
    int64_t length,
    sqlite3_stmt** statement,
    int32_t* hasMore
) {
    const char* tail = NULL;
    *hasMore = 0;
    int status = sqlite3_prepare_v3(database, sql, (int)length, 0, statement, &tail);
    if (status != SQLITE_OK || !*statement || !tail) {
        return status;
    }
    int64_t remaining = length - (int64_t)(tail - sql);
    if (remaining > 0) {
        sqlite3_stmt* next = NULL;
        int nextStatus = sqlite3_prepare_v3(database, tail, (int)remaining, 0, &next, NULL);
        if (next || nextStatus != SQLITE_OK) {
            *hasMore = 1;
        }
        sqlite3_finalize(next);
    }
    return status;
}

void yumiSQLiteLock(sqlite3* database) {
    sqlite3_mutex_enter(sqlite3_db_mutex(database));
}

void yumiSQLiteUnlock(sqlite3* database) {
    sqlite3_mutex_leave(sqlite3_db_mutex(database));
}

int32_t yumiSQLiteBindNull(sqlite3_stmt* statement, int32_t index) {
    return sqlite3_bind_null(statement, index);
}

int32_t yumiSQLiteBindInt64(sqlite3_stmt* statement, int32_t index, int64_t value) {
    return sqlite3_bind_int64(statement, index, value);
}

int32_t yumiSQLiteBindDouble(sqlite3_stmt* statement, int32_t index, double value) {
    return sqlite3_bind_double(statement, index, value);
}

int32_t yumiSQLiteBindText(
    sqlite3_stmt* statement,
    int32_t index,
    const char* text,
    int64_t length
) {
    return sqlite3_bind_text64(
        statement,
        index,
        text,
        (sqlite3_uint64)length,
        SQLITE_TRANSIENT,
        SQLITE_UTF8
    );
}

int32_t yumiSQLiteBindBlob(
    sqlite3_stmt* statement,
    int32_t index,
    const void* bytes,
    int64_t length
) {
    if (length == 0) {
        return sqlite3_bind_zeroblob(statement, index, 0);
    }
    return sqlite3_bind_blob64(statement, index, bytes, (sqlite3_uint64)length, SQLITE_TRANSIENT);
}

int32_t yumiSQLiteColumnCount(sqlite3_stmt* statement) {
    return sqlite3_column_count(statement);
}

int32_t yumiSQLiteReadType(sqlite3_stmt* statement, int64_t index) {
    if (index < 0 || index >= sqlite3_column_count(statement)) {
        return 0;
    }
    return sqlite3_column_type(statement, (int)index);
}

int64_t yumiSQLiteReadInt64(sqlite3_stmt* statement, int64_t index, int32_t* type) {
    *type = yumiSQLiteReadType(statement, index);
    if (*type != SQLITE_INTEGER) {
        return 0;
    }
    return sqlite3_column_int64(statement, (int)index);
}

double yumiSQLiteReadDouble(sqlite3_stmt* statement, int64_t index, int32_t* type) {
    *type = yumiSQLiteReadType(statement, index);
    if (*type != SQLITE_FLOAT && *type != SQLITE_INTEGER) {
        return 0;
    }
    return sqlite3_column_double(statement, (int)index);
}

const char* yumiSQLiteReadText(
    sqlite3_stmt* statement,
    int64_t index,
    int32_t* type,
    int64_t* length
) {
    *type = yumiSQLiteReadType(statement, index);
    *length = 0;
    if (*type != SQLITE_TEXT) {
        return NULL;
    }
    const unsigned char* text = sqlite3_column_text(statement, (int)index);
    *length = sqlite3_column_bytes(statement, (int)index);
    return (const char*)text;
}

const void* yumiSQLiteReadBlob(
    sqlite3_stmt* statement,
    int64_t index,
    int32_t* type,
    int64_t* length
) {
    *type = yumiSQLiteReadType(statement, index);
    *length = 0;
    if (*type != SQLITE_BLOB) {
        return NULL;
    }
    const void* bytes = sqlite3_column_blob(statement, (int)index);
    *length = sqlite3_column_bytes(statement, (int)index);
    return bytes;
}
