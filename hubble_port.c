#include <hubble/port/sys.h>
#include "sl_sleeptimer.h"
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>

#define _KEY_BITS_LEN (CONFIG_HUBBLE_KEY_SIZE * 8)

uint64_t hubble_uptime_get(void) {
    uint64_t ticks = sl_sleeptimer_get_tick_count64();
    uint64_t time_ms;
    if (sl_sleeptimer_tick64_to_ms(ticks, &time_ms) != SL_STATUS_OK) {
        return 0;
    }
    return time_ms;
}

int hubble_log(enum hubble_log_level level, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    FILE *stream = (level >= HUBBLE_LOG_WARNING) ? stderr : stdout;

    const char *tag =
        (level == HUBBLE_LOG_DEBUG) ? "DEBUG" :
        (level == HUBBLE_LOG_INFO)  ? "INFO"  :
        (level == HUBBLE_LOG_WARNING)  ? "WARN"  : "ERROR";

    int n = fprintf(stream, "\n\r[%s] ", tag);      // prefix
    if (n < 0) { va_end(args); return n; }

    int m = vfprintf(stream, format, args);     // caller’s message

    va_end(args);

    return (m < 0) ? m : n + m;
}
