#ifdef DEBUG
#define TRACE { \
     logf("%s @ %u", __FILE__, __LINE__); \
     fflush(stdout); \
}
#define TRACEF(a) { \
     logf("%s @ %u", __FILE__, __LINE__); \
     logf a; \
     fflush(stdout); \
}
#else
#define TRACE
#define TRACEF(a)
#endif
