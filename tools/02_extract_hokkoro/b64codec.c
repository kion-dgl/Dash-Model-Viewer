/*
 * The MIT License (MIT)
 * Copyright (c) 2018, archmirak
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include <inttypes.h>

uint8_t b64enc(const uint8_t *wrd, const size_t wrdlen, uint8_t *b64wrd, const size_t b64maxlen, size_t *b64wrdlen) {
    static const uint8_t b64table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    static const uint8_t b64padding[] = {0, 2, 1};
    *b64wrdlen = (((4 * (wrdlen - 1) / 3) + 3) & ~0x03) + 1;
    if (*b64wrdlen > b64maxlen) return 1;

    for (size_t i = 0, j = 0; i < wrdlen - 1;) {
        uint32_t octeta = i < wrdlen - 1 ? wrd[i++] << 0x10 : 0;
        uint32_t octetb = i < wrdlen - 1 ? wrd[i++] << 0x08 : 0;
        uint32_t octetc = i < wrdlen - 1 ? wrd[i++] : 0;
        uint32_t b64sextet = octeta | octetb | octetc;
        
        if (j < *b64wrdlen - 1) b64wrd[j++] = b64table[b64sextet >> 0x12];
        if (j < *b64wrdlen - 1) b64wrd[j++] = b64table[(b64sextet >> 0x0c) & 0x3f];
        if (j < *b64wrdlen - 1) b64wrd[j++] = b64table[(b64sextet >> 0x06) & 0x3f];
        if (j < *b64wrdlen - 1) b64wrd[j++] = b64table[b64sextet & 0x3f];
    }

    for (uint8_t i = 0; i < b64padding[(wrdlen - 1) % 3]; i++) b64wrd[*b64wrdlen - 2 - i] = 0x3d;
    b64wrd[*b64wrdlen - 1] = 0x00;

    return 0;
}

uint8_t b64dec(const uint8_t *b64wrd, const size_t b64wrdlen, uint8_t *wrd, const size_t maxlen, size_t *wrdlen) {
    if ((b64wrdlen - 1) % 4 != 0) return 1;
    static const uint8_t asciitable[] = {
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
        64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
        64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
    };
    const uint8_t b64padding = (b64wrd[b64wrdlen - 2] ^ 0x3d ? 0 : 1) + (b64wrd[b64wrdlen - 3] ^ 0x3d ? 0 : 1);
    *wrdlen = b64wrdlen / 4 * 3 + b64padding + 1;
    if (*wrdlen > maxlen) return 2;

    for (size_t i = 0, j = 0; i < b64wrdlen - 1;) {
        uint32_t sexteta = i < b64wrdlen - 1 ? (b64wrd[i] != 0x3d ? asciitable[b64wrd[i++]] << 0x12 : i++) : 0;
        uint32_t sextetb = i < b64wrdlen - 1 ? (b64wrd[i] != 0x3d ? asciitable[b64wrd[i++]] << 0x0c : i++) : 0;
        uint32_t sextetc = i < b64wrdlen - 1 ? (b64wrd[i] != 0x3d ? asciitable[b64wrd[i++]] << 0x06 : i++) : 0;
        uint32_t sextetd = i < b64wrdlen - 1 ? (b64wrd[i] != 0x3d ? asciitable[b64wrd[i++]] : i++) : 0;
        uint32_t octet = sexteta | sextetb | sextetc | sextetd;
        
        if (j < *wrdlen - 1) wrd[j++] = octet >> 0x10;
        if (j < *wrdlen - 1) wrd[j++] = (octet >> 0x08) & 0xff;
        if (j < *wrdlen - 1) wrd[j++] = octet & 0xff;
    }
    wrd[*wrdlen - 1] = 0x00;

    return 0;
}
