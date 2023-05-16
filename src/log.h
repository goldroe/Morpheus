#include <stdio.h>

#ifdef DEVELOPER
#define __LOG(CLR, FMT, ...) printf("\033[%sm" FMT "\033[m\n", CLR, ##__VA_ARGS__)
#else
#define __LOG(CLR, FMT, ...) 
#endif

#define __COLOR_RED "1;31"
#define __COLOR_GREEN "0;32"
#define __COLOR_BLUE "1;34"
#define __COLOR_CYAN "0;36"
#define __COLOR_YELLOW "0;33"
#define __COLOR_WHITE "0;37"

#define LOG(FMT, ...)     __LOG(__COLOR_WHITE,  FMT, ##__VA_ARGS__)
#define ALERT(FMT, ...)   __LOG(__COLOR_RED,    FMT, ##__VA_ARGS__)
#define WARNING(FMT, ...) __LOG(__COLOR_YELLOW, FMT, ##__VA_ARGS__)


#ifdef DEVELOPER
#define ASSERT(X) { \
   if (!(X)) {         \
       __debugbreak(); \
   }                   \
}
#else
#define ASSERT(X)
#endif
