#ifndef MATHS_H
#define MATHS_H

#define dd decimal_digit
#define pot power_of_ten

#include <stdint.h>

/// returns ten to the power of n
uint32_t power_of_ten(uint32_t n) {
    if (n == 0) return 1;
    uint32_t r = 10;
    for (uint32_t i = 0; i < n-1; i++) {r *= 10;}
    return r;
}

/// returns decimal digit at the nth position in given number
uint8_t decimal_digit(uint32_t num, uint32_t n) {
    uint32_t r;
    r = num / power_of_ten(n);
    r = r % 10;
    return (uint8_t) r;
}

#endif //MATHS_H
