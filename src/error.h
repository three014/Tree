#ifndef __ERROR_H
#define __ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

#define handle_error(msg)                                                      \
    do {                                                                       \
        perror(msg);                                                           \
        exit(1);                                                               \
    } while (0)

void dbg(const char *fmt, ...) {
#ifdef DO_DEBUG
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
#endif
}

#define dbg_line(fmt, ...)                                                     \
    do {                                                                       \
        dbg("[%s:%lu] " fmt, __FILE__, __LINE__, __VA_ARGS__);                 \
    } while (0)

#define dbg_line_no_fmt(msg) dbg_line(msg "%s", "")

#ifdef __cplusplus
}
#endif

#endif