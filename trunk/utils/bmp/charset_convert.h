#ifndef CHARSET_CONVERT_H
#define CHARSET_CONVERT_H
#include <sys/types.h>

//public
int charset_convert_UTF8_TO_GB2312(char *in_buf, size_t in_left, char *out_buf, size_t out_left);
int charset_convert_GB2312_HALF_TO_FULL(char *bdest, size_t dest_len, char *bsrc, size_t src_len);

//private
int charset_convert(const char *from_charset, const char *to_charset,
                           char *in_buf, size_t in_left, char *out_buf, size_t out_left);

#endif
