
#include <cstdlib>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <cfloat>
// #include <gc.h>

#include "BxCore/BxCore.h"
#include "BxCore/BxNumber.h"
#include "BxCore/BxString.h"

// *************************************************************************************************

static void
_numberFormattingFailed(const char* reason) {
    fprintf(stderr, "%s\n", reason);
    exit(1);
}

static const BrixStringData*
_asciiFormat(const char* format, ...) {
    char buffer[128];
    va_list args;
    va_start(args, format);
    int formattedLength = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (formattedLength < 0) {
        _numberFormattingFailed("Number formatting failed");
    }

    size_t length = (size_t)formattedLength;
    if (length < sizeof(buffer)) {
        return brixStringCreateASCII(buffer, length);
    }

    char* fullBuffer = (char*)malloc(length + 1);
    if (!fullBuffer) {
        _numberFormattingFailed("Number formatting allocation failed");
    }

    va_start(args, format);
    int fullLength = vsnprintf(fullBuffer, length + 1, format, args);
    va_end(args);
    if (fullLength != formattedLength) {
        free(fullBuffer);
        _numberFormattingFailed("Number formatting retry failed");
    }

    const BrixStringData* result = brixStringCreateASCII(fullBuffer, length);
    free(fullBuffer);
    return result;
}

// *************************************************************************************************

extern "C" const BrixStringData*
brixIntToString(int num, int padding) {
    return _asciiFormat("%0*d", padding, num);
}

extern "C" const BrixStringData*
brixUintToString(unsigned int num, int padding) {
    return _asciiFormat("%0*u", padding, num);
}

extern "C" const BrixStringData*
brixLonglongToString(long long num, int padding) {
    return _asciiFormat("%0*lld", padding, num);
}

extern "C" const BrixStringData*
brixUlonglongToString(unsigned long long num, int padding) {
    return _asciiFormat("%0*llu", padding, num);
}

extern "C" const BrixStringData*
brixFloatToString(float num) {
    if (isfinite(num)) {
        char buffer[32];
        for (int precision = 6; precision < 9; ++precision) {
            snprintf(buffer, sizeof(buffer), "%.*g", precision, (double)num);
            if (strtof(buffer, nullptr) == num) {
                return brixStringCreateASCII(buffer, strlen(buffer));
            }
        }
    }
    return _asciiFormat("%.9g", num);
}

extern "C" const BrixStringData*
brixDoubleToString(double num) {
    if (isfinite(num)) {
        char buffer[40];
        for (int precision = 15; precision < 17; ++precision) {
            snprintf(buffer, sizeof(buffer), "%.*g", precision, num);
            if (strtod(buffer, nullptr) == num) {
                return brixStringCreateASCII(buffer, strlen(buffer));
            }
        }
    }
    return _asciiFormat("%.17g", num);
}

extern "C" const BrixStringData*
brixDoubleToFixedString(double num, int digits, int trims) {
    if (!isfinite(num)) {
        return brixDoubleToString(num);
    }
    if (digits < 0) {
        digits = 0;
    } else if (digits > DBL_DIG) {
        digits = DBL_DIG;
    }
    double scale = pow(10.0, digits);
    double scaled = round(num * scale);
    double rounded = isfinite(scaled) ? scaled / scale : num;
    char buffer[DBL_MAX_10_EXP + DBL_DIG + 8];
    int length = snprintf(buffer, sizeof(buffer), "%.*f", digits, rounded);
    if (length < 0 || (size_t)length >= sizeof(buffer)) {
        _numberFormattingFailed("Number formatting failed");
    }
    if (trims && strchr(buffer, '.')) {
        while (buffer[length - 1] == '0') {
            length -= 1;
        }
        if (buffer[length - 1] == '.') {
            length -= 1;
        }
    }
    buffer[length] = '\0';
    const char* start = buffer;
    if (buffer[0] == '-' && strspn(buffer + 1, "0.") == (size_t)(length - 1)) {
        start += 1;
        length -= 1;
    }
    return brixStringCreateASCII(start, (size_t)length);
}

extern "C" const BrixStringData*
brixToHex(unsigned long long num, int size) {
    return _asciiFormat("0x%0*llx", size, num);
}

// *************************************************************************************************

extern "C" double
brixModf64(double n, double* rem) {
    return modf(n, rem);
}
