/*
 */

#ifndef CHFORMAT_H
#define CHFORMAT_H

#ifdef __GNUC__
#define CHK_FORMAT_1 __attribute__ ((format(printf, 1, 2)))
#define CHK_FORMAT_2 __attribute__ ((format(printf, 2, 3)))
#define CHK_FORMAT_3 __attribute__ ((format(printf, 3, 4)))
#else
#define CHK_FORMAT_1  
#define CHK_FORMAT_2
#define CHK_FORMAT_3
#endif

#endif
