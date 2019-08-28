#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "crc_test.h"

int main(void) {
    unsigned char data[31];
    srandom(time(NULL));
    {
        uint64_t ran = 1;
        size_t n = sizeof(data);
        do {
            if (ran < 0x100)
                ran = (ran << 31) + random();
            data[--n] = ran;
            ran >>= 8;
        } while (n);
    }
    uintmax_t init, crc;

    // crc16buypass
    init = crc16buypass_bit(0, NULL, 0);
    if (crc16buypass_bit(init, "123456789", 9) != 0xfee8)
        fputs("bit-wise mismatch for crc16buypass\n", stderr);
    crc = crc16buypass_bit(init, data + 1, sizeof(data) - 1);
    if (crc16buypass_bit(init, "\xda", 1) !=
        crc16buypass_rem(crc16buypass_rem(init, 0xda, 3), 0xd0, 5))
        fputs("small bits mismatch for crc16buypass\n", stderr);
    if (crc16buypass_byte(0, NULL, 0) != init ||
        crc16buypass_byte(init, "123456789", 9) != 0xfee8 ||
        crc16buypass_byte(init, data + 1, sizeof(data) - 1) != crc)
        fputs("byte-wise mismatch for crc16buypass\n", stderr);
    if (crc16buypass_word(0, NULL, 0) != init ||
        crc16buypass_word(init, "123456789", 9) != 0xfee8 ||
        crc16buypass_word(init, data + 1, sizeof(data) - 1) != crc)
        fputs("word-wise mismatch for crc16buypass\n", stderr);

    return 0;
}
