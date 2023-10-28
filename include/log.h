#ifndef __LOG_H
#define __LOG_H

#include <stdarg.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ERROR   "Error: " /* Level 1 */
#define WARNING "Warn:  " /* Level 2 */
#define INFO    "Info:  " /* Level 3 */
#define DEBUG   "Debug: " /* Level 4 */

#ifndef LOG_LEVEL
#define LOG_LEVEL 0
#endif

#define __logln_info(msg) __logln_info_fmt(msg "%s", "")
#define __logln_err(msg) __logln_err_fmt(msg "%s", "")
#define __logln_warn(msg) __logln_warn_fmt(msg "%s", "")
#define __logln_dbg(msg) __logln_dbg_fmt(msg "%s", "")

#define __logln_info_fmt(msg, ...)  \
    do {                            \
        switch (LOG_LEVEL) {        \
        case 0:                     \
        case 1:                     \
        case 2:                     \
            break;                  \
        case 3:                     \
        case 4:                     \
            dbg(INFO msg "\n", __VA_ARGS__); \
        default:                    \
            break;                  \
        }                           \
    } while (0)

#define __logln_warn_fmt(msg, ...)  \
    do {                            \
        switch (LOG_LEVEL) {        \
        case 0:                     \
        case 1:                     \
            break;                  \
        case 2:                     \
        case 3:                     \
        case 4:                     \
            dbg(WARNING msg "\n", __VA_ARGS__); \
        default:                    \
            break;                  \
        }                           \
    } while (0)

#define __logln_err_fmt(msg, ...)   \
    do {                            \
        switch (LOG_LEVEL) {        \
        case 0:                     \
            break;                  \
        case 1:                     \
        case 2:                     \
        case 3:                     \
        case 4:                     \
            dbg(ERROR msg "\n", __VA_ARGS__); \
        default:                    \
            break;                  \
        }                           \
    } while (0)

#define __logln_dbg_fmt(msg, ...)   \
    do {                            \
        switch (LOG_LEVEL) {        \
        case 0:                     \
        case 1:                     \
        case 2:                     \
        case 3:                     \
            break;                  \
        case 4:                     \
            dbg(DEBUG msg "\n", __VA_ARGS__); \
        default:                    \
            break;                  \
        }                           \
    } while (0)



/// Prints a formatted message to
/// stderr, pretty much like 'fprintf'
static void dbg(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

#ifdef __cplusplus
}
#endif

#endif
