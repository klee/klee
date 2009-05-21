typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

uint32_t util_make_and_i1(uint32_t a, uint32_t b);
uint32_t util_make_or_i1(uint32_t a, uint32_t b);

uint16_t util_make_concat2(uint8_t a, uint8_t b);
uint32_t util_make_concat4(uint8_t a, uint8_t b, 
                           uint8_t c, uint8_t d);
uint64_t util_make_concat8(uint8_t a, uint8_t b,
                           uint8_t c, uint8_t d,
                           uint8_t e, uint8_t f,
                           uint8_t g, uint8_t h);
uint32_t util_make_select(uint32_t cond, uint32_t true, uint32_t false);
