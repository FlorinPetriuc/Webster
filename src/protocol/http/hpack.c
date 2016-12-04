/*
 * Copyright (C) 2016 Florin Petriuc. All rights reserved.
 * Initial release: Florin Petriuc <petriuc.florin@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include "../../main.h"

#define HTTP2_GET           2
#define HTTP2_POST          3
#define HTTP2_CONTENT_TYPE  31

enum http_request_type_t hpack_resolve_method(const unsigned char *payload)
{
    uint32_t i = 9;
    uint32_t len;

    unsigned char indexed_value;
    unsigned char value_len;

    len = (payload[0] << 16) | (payload[1] << 8) | payload[2];

    while(i < len + 9)
    {
        if(payload[i] & 0x80)
        {
            indexed_value = (payload[i] & 0x7F);

            switch(indexed_value)
            {
                case HTTP2_GET: return HTTP_GET;
            }

            ++i;

            continue;
        }

        if(payload[i] & 0x40)
        {
            indexed_value = (payload[i] & 0x3F);
            ++i;

            value_len = (payload[i] & 0x7F);

            i += value_len + 1;

            if(indexed_value)
            {
                continue;
            }

            value_len = (payload[i] & 0x7F);

            i += value_len + 1;

            continue;
        }

        if((payload[i] >> 4) == 0 ||
           (payload[i] >> 4) == 1)
        {
            indexed_value = (payload[i] & 0x0F);
            ++i;

            value_len = (payload[i] & 0x7F);

            i += value_len + 1;

            if(indexed_value)
            {
                continue;
            }

            value_len = (payload[i] & 0x7F);

            i += value_len + 1;

            continue;
        }

        logWrite(LOG_TYPE_INFO, "Got unsupported hpack method", 0);

        return HTTP_UNSUPPORTED_METHOD;
    }

    return HTTP_UNSUPPORTED_METHOD;
}

static unsigned char get_bit_at(const unsigned char v, const unsigned char pos)
{
    unsigned char ret = 0;

    ret = (v >> (8 - pos - 1)) & 1;

    return ret;
}

static char *huffman_decode(const unsigned char *value, const unsigned char len)
{
    uint32_t char_encoded = 0;

    uint16_t i = 0;
    uint16_t vIdx;

    unsigned char bits_used = 0;

    char decoded_str[768];

    char *ret;

    logWrite(LOG_TYPE_INFO, "Decoding %hhu bytes, first byte is %hhu", 2, len, value[0]);

    for(vIdx = 0; vIdx < len * 8; ++vIdx)
    {
        char_encoded = (char_encoded << 1) | get_bit_at(value[vIdx / 8], vIdx & 7);

        ++bits_used;

        if(i == sizeof(decoded_str))
        {
            break;
        }

        switch(bits_used)
        {
            case 5:
            {
                if(char_encoded >= 0x0 && char_encoded <= 0x2)
                {
                    decoded_str[i++] = char_encoded - 0x0 + 48;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x3)
                {
                    decoded_str[i++] = 97;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x4)
                {
                    decoded_str[i++] = 99;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x5)
                {
                    decoded_str[i++] = 101;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x6)
                {
                    decoded_str[i++] = 105;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x7)
                {
                    decoded_str[i++] = 111;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0x8 && char_encoded <= 0x9)
                {
                    decoded_str[i++] = char_encoded - 0x8 + 115;

                    char_encoded = 0;
                    bits_used = 0;
                }
            }
            break;

            case 6:
            {
                if(char_encoded == 0x14)
                {
                    decoded_str[i++] = 32;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x15)
                {
                    decoded_str[i++] = 37;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0x16 && char_encoded <= 0x18)
                {
                    decoded_str[i++] = char_encoded - 0x16 + 45;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0x19 && char_encoded <= 0x1f)
                {
                    decoded_str[i++] = char_encoded - 0x19 + 51;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x20)
                {
                    decoded_str[i++] = 61;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x21)
                {
                    decoded_str[i++] = 65;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x22)
                {
                    decoded_str[i++] = 95;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x23)
                {
                    decoded_str[i++] = 98;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x24)
                {
                    decoded_str[i++] = 100;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0x25 && char_encoded <= 0x27)
                {
                    decoded_str[i++] = char_encoded - 0x25 + 102;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0x28 && char_encoded <= 0x2a)
                {
                    decoded_str[i++] = char_encoded - 0x28 + 108;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x2b)
                {
                    decoded_str[i++] = 112;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x2c)
                {
                    decoded_str[i++] = 114;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x2d)
                {
                    decoded_str[i++] = 117;

                    char_encoded = 0;
                    bits_used = 0;
                }
            }
            break;

            case 7:
            {
                if(char_encoded == 0x5c)
                {
                    decoded_str[i++] = 58;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0x5d && char_encoded <= 0x72)
                {
                    decoded_str[i++] = char_encoded - 0x5d + 66;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x73)
                {
                    decoded_str[i++] = 89;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0x74 && char_encoded <= 0x75)
                {
                    decoded_str[i++] = char_encoded - 0x74 + 106;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x76)
                {
                    decoded_str[i++] = 113;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0x77 && char_encoded <= 0x7b)
                {
                    decoded_str[i++] = char_encoded - 0x77 + 118;

                    char_encoded = 0;
                    bits_used = 0;
                }
            }
            break;

            case 8:
            {
                if(char_encoded == 0xf8)
                {
                    decoded_str[i++] = 38;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0xf9)
                {
                    decoded_str[i++] = 42;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0xfa)
                {
                    decoded_str[i++] = 44;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0xfb)
                {
                    decoded_str[i++] = 59;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0xfc)
                {
                    decoded_str[i++] = 88;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0xfd)
                {
                    decoded_str[i++] = 90;

                    char_encoded = 0;
                    bits_used = 0;
                }
            }
            break;

            case 10:
            {
                if(char_encoded >= 0x3f8 && char_encoded <= 0x3f9)
                {
                    decoded_str[i++] = char_encoded - 0x3f8 + 33;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0x3fa && char_encoded <= 0x3fb)
                {
                    decoded_str[i++] = char_encoded - 0x3fa + 40;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x3fc)
                {
                    decoded_str[i++] = 63;

                    char_encoded = 0;
                    bits_used = 0;
                }
            }
            break;

            case 11:
            {
                if(char_encoded == 0x7fa)
                {
                    decoded_str[i++] = 39;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x7fb)
                {
                    decoded_str[i++] = 43;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x7fc)
                {
                    decoded_str[i++] = 124;

                    char_encoded = 0;
                    bits_used = 0;
                }
            }
            break;

            case 12:
            {
                if(char_encoded == 0xffa)
                {
                    decoded_str[i++] = 35;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0xffb)
                {
                    decoded_str[i++] = 62;

                    char_encoded = 0;
                    bits_used = 0;
                }
            }
            break;

            case 13:
            {
                if(char_encoded == 0x1ff8)
                {
                    decoded_str[i++] = 0;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x1ff9)
                {
                    decoded_str[i++] = 36;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x1ffa)
                {
                    decoded_str[i++] = 64;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x1ffb)
                {
                    decoded_str[i++] = 91;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x1ffc)
                {
                    decoded_str[i++] = 93;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x1ffd)
                {
                    decoded_str[i++] = 126;

                    char_encoded = 0;
                    bits_used = 0;
                }
            }
            break;

            case 14:
            {
                if(char_encoded == 0x3ffc)
                {
                    decoded_str[i++] = 94;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x3ffd)
                {
                    decoded_str[i++] = 125;

                    char_encoded = 0;
                    bits_used = 0;
                }
            }
            break;

            case 15:
            {
                if(char_encoded == 0x7ffc)
                {
                    decoded_str[i++] = 60;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x7ffd)
                {
                    decoded_str[i++] = 96;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x7ffe)
                {
                    decoded_str[i++] = 123;

                    char_encoded = 0;
                    bits_used = 0;
                }
            }
            break;

            case 19:
            {
                if(char_encoded == 0x7fff0)
                {
                    decoded_str[i++] = 92;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x7fff1)
                {
                    decoded_str[i++] = 195;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x7fff2)
                {
                    decoded_str[i++] = 207;

                    char_encoded = 0;
                    bits_used = 0;
                }
            }
            break;

            case 20:
            {
                if(char_encoded == 0xfffe6)
                {
                    decoded_str[i++] = 128;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0xfffe7 && char_encoded <= 0xfffe8)
                {
                    decoded_str[i++] = char_encoded - 0xfffe7 + 130;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0xfffe9)
                {
                    decoded_str[i++] = 162;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0xfffea)
                {
                    decoded_str[i++] = 184;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0xfffeb)
                {
                    decoded_str[i++] = 194;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0xfffec)
                {
                    decoded_str[i++] = 224;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0xfffed)
                {
                    decoded_str[i++] = 226;

                    char_encoded = 0;
                    bits_used = 0;
                }
            }
            break;

            case 21:
            {
                if(char_encoded == 0x1fffdc)
                {
                    decoded_str[i++] = 153;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x1fffdd)
                {
                    decoded_str[i++] = 161;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x1fffde)
                {
                    decoded_str[i++] = 167;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x1fffdf)
                {
                    decoded_str[i++] = 172;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0x1fffe0 && char_encoded <= 0x1fffe1)
                {
                    decoded_str[i++] = char_encoded - 0x1fffe0 + 177;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x1fffe2)
                {
                    decoded_str[i++] = 179;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x1fffe3)
                {
                    decoded_str[i++] = 209;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0x1fffe4 && char_encoded <= 0x1fffe5)
                {
                    decoded_str[i++] = char_encoded - 0x1fffe4 + 216;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x1fffe6)
                {
                    decoded_str[i++] = 227;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0x1fffe7 && char_encoded <= 0x1fffe8)
                {
                    decoded_str[i++] = char_encoded - 0x1fffe7 + 229;

                    char_encoded = 0;
                    bits_used = 0;
                }
            }
            break;

            case 22:
            {
                if(char_encoded == 0x3fffd2)
                {
                    decoded_str[i++] = 129;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0x3fffd3 && char_encoded <= 0x3fffd5)
                {
                    decoded_str[i++] = char_encoded - 0x3fffd3 + 132;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x3fffd6)
                {
                    decoded_str[i++] = 136;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x3fffd7)
                {
                    decoded_str[i++] = 146;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x3fffd8)
                {
                    decoded_str[i++] = 154;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x3fffd9)
                {
                    decoded_str[i++] = 156;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x3fffda)
                {
                    decoded_str[i++] = 160;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0x3fffdb && char_encoded <= 0x3fffdc)
                {
                    decoded_str[i++] = char_encoded - 0x3fffdb + 163;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0x3fffdd && char_encoded <= 0x3fffde)
                {
                    decoded_str[i++] = char_encoded - 0x3fffdd + 169;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x3fffdf)
                {
                    decoded_str[i++] = 173;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x3fffe0)
                {
                    decoded_str[i++] = 178;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x3fffe1)
                {
                    decoded_str[i++] = 181;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0x3fffe2 && char_encoded <= 0x3fffe4)
                {
                    decoded_str[i++] = char_encoded - 0x3fffe2 + 185;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0x3fffe5 && char_encoded <= 0x3fffe6)
                {
                    decoded_str[i++] = char_encoded - 0x3fffe5 + 189;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x3fffe7)
                {
                    decoded_str[i++] = 196;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x3fffe8)
                {
                    decoded_str[i++] = 198;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x3fffe9)
                {
                    decoded_str[i++] = 228;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0x3fffea && char_encoded <= 0x3fffeb)
                {
                    decoded_str[i++] = char_encoded - 0x3fffea + 232;

                    char_encoded = 0;
                    bits_used = 0;
                }
            }
            break;

            case 23:
            {
                if(char_encoded == 0x7fffd8)
                {
                    decoded_str[i++] = 1;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x7fffd9)
                {
                    decoded_str[i++] = 135;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0x7fffda && char_encoded <= 0x7fffde)
                {
                    decoded_str[i++] = char_encoded - 0x7fffda + 137;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x7fffdf)
                {
                    decoded_str[i++] = 143;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x7fffe0)
                {
                    decoded_str[i++] = 147;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0x7fffe1 && char_encoded <= 0x7fffe4)
                {
                    decoded_str[i++] = char_encoded - 0x7fffe1 + 149;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x7fffe5)
                {
                    decoded_str[i++] = 155;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0x7fffe6 && char_encoded <= 0x7fffe7)
                {
                    decoded_str[i++] = char_encoded - 0x7fffe6 + 157;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0x7fffe8 && char_encoded <= 0x7fffe9)
                {
                    decoded_str[i++] = char_encoded - 0x7fffe8 + 165;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x7fffea)
                {
                    decoded_str[i++] = 168;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0x7fffeb && char_encoded <= 0x7fffec)
                {
                    decoded_str[i++] = char_encoded - 0x7fffeb + 174;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x7fffed)
                {
                    decoded_str[i++] = 180;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0x7fffee && char_encoded <= 0x7fffef)
                {
                    decoded_str[i++] = char_encoded - 0x7fffee + 182;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x7ffff0)
                {
                    decoded_str[i++] = 188;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x7ffff1)
                {
                    decoded_str[i++] = 191;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x7ffff2)
                {
                    decoded_str[i++] = 197;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x7ffff3)
                {
                    decoded_str[i++] = 231;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x7ffff4)
                {
                    decoded_str[i++] = 239;

                    char_encoded = 0;
                    bits_used = 0;
                }
            }
            break;

            case 24:
            {
                if(char_encoded == 0xffffea)
                {
                    decoded_str[i++] = 9;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0xffffeb)
                {
                    decoded_str[i++] = 142;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0xffffec && char_encoded <= 0xffffed)
                {
                    decoded_str[i++] = char_encoded - 0xffffec + 144;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0xffffee)
                {
                    decoded_str[i++] = 148;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0xffffef)
                {
                    decoded_str[i++] = 159;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0xfffff0)
                {
                    decoded_str[i++] = 171;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0xfffff1)
                {
                    decoded_str[i++] = 206;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0xfffff2)
                {
                    decoded_str[i++] = 215;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0xfffff3)
                {
                    decoded_str[i++] = 225;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0xfffff4 && char_encoded <= 0xfffff5)
                {
                    decoded_str[i++] = char_encoded - 0xfffff4 + 236;

                    char_encoded = 0;
                    bits_used = 0;
                }
            }
            break;

            case 25:
            {
                if(char_encoded == 0x1ffffec)
                {
                    decoded_str[i++] = 199;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x1ffffed)
                {
                    decoded_str[i++] = 207;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0x1ffffee && char_encoded <= 0x1ffffef)
                {
                    decoded_str[i++] = char_encoded - 0x1ffffee + 234;

                    char_encoded = 0;
                    bits_used = 0;
                }
            }
            break;

            case 26:
            {
                if(char_encoded >= 0x3ffffe0 && char_encoded <= 0x3ffffe1)
                {
                    decoded_str[i++] = char_encoded - 0x3ffffe0 + 192;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0x3ffffe2 && char_encoded <= 0x3ffffe4)
                {
                    decoded_str[i++] = char_encoded - 0x3ffffe2 + 200;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x3ffffe5)
                {
                    decoded_str[i++] = 205;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x3ffffe6)
                {
                    decoded_str[i++] = 210;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x3ffffe7)
                {
                    decoded_str[i++] = 213;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0x3ffffe8 && char_encoded <= 0x3ffffe9)
                {
                    decoded_str[i++] = char_encoded - 0x3ffffe8 + 218;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x3ffffea)
                {
                    decoded_str[i++] = 238;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x3ffffeb)
                {
                    decoded_str[i++] = 240;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0x3ffffec && char_encoded <= 0x3ffffed)
                {
                    decoded_str[i++] = char_encoded - 0x3ffffec + 242;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x3ffffee)
                {
                    decoded_str[i++] = 255;

                    char_encoded = 0;
                    bits_used = 0;
                }
            }
            break;

            case 27:
            {
                if(char_encoded >= 0x7ffffde && char_encoded <= 0x7ffffdf)
                {
                    decoded_str[i++] = char_encoded - 0x7ffffde + 204;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0x7ffffe0 && char_encoded <= 0x7ffffe1)
                {
                    decoded_str[i++] = char_encoded - 0x7ffffe0 + 211;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x7ffffe2)
                {
                    decoded_str[i++] = 214;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0x7ffffe3 && char_encoded <= 0x7ffffe5)
                {
                    decoded_str[i++] = char_encoded - 0x7ffffe3 + 221;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x7ffffe6)
                {
                    decoded_str[i++] = 241;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0x7ffffe7 && char_encoded <= 0x7ffffeb)
                {
                    decoded_str[i++] = char_encoded - 0x7ffffe7 + 244;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0x7ffffec && char_encoded <= 0x7fffff0)
                {
                    decoded_str[i++] = char_encoded - 0x7ffffec + 250;

                    char_encoded = 0;
                    bits_used = 0;
                }
            }
            break;

            case 28:
            {
                if(char_encoded >= 0xfffffe2 && char_encoded <= 0xfffffe8)
                {
                    decoded_str[i++] = char_encoded - 0xfffffe2 + 2;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0xfffffe9 && char_encoded <= 0xfffffea)
                {
                    decoded_str[i++] = char_encoded - 0xfffffe9 + 11;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0xfffffeb && char_encoded <= 0xffffff2)
                {
                    decoded_str[i++] = char_encoded - 0xfffffeb + 14;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded >= 0xffffff3 && char_encoded <= 0xffffffb)
                {
                    decoded_str[i++] = char_encoded - 0xffffff3 + 23;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0xffffffc)
                {
                    decoded_str[i++] = 127;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0xffffffd)
                {
                    decoded_str[i++] = 220;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0xffffffe)
                {
                    decoded_str[i++] = 249;

                    char_encoded = 0;
                    bits_used = 0;
                }
            }
            break;

            case 30:
            {
                if(char_encoded == 0x3ffffffc)
                {
                    decoded_str[i++] = 10;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x3ffffffd)
                {
                    decoded_str[i++] = 13;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x3ffffffe)
                {
                    decoded_str[i++] = 22;

                    char_encoded = 0;
                    bits_used = 0;
                }
                else if(char_encoded == 0x3fffffff)
                {
                    decoded_str[i++] = 0;

                    char_encoded = 0;
                    bits_used = 0;
                }
            }
            break;
        }
    }

    ret = xmalloc(i + 1);
    memcpy(ret, decoded_str, i);
    ret[i] = '\0';

    return ret;
}

static unsigned int hpack_process_content_length(const unsigned char *content_length)
{
    unsigned int ret = 0;

    unsigned char huffman_coded;
    unsigned char value_len;
    unsigned char i;

    char *huffman_decoded = NULL;

    huffman_coded = content_length[0] & 0x80;
    value_len = content_length[0] & 0x7F;

    if(huffman_coded)
    {
        huffman_decoded = huffman_decode(content_length + 1, value_len);

        if(huffman_decoded == NULL)
        {
            return 0;
        }

        sscanf(huffman_decoded, "%u", &ret);
        free(huffman_decoded);
    }
    else
    {
        for(i = 0; i < value_len; ++i)
        {
            ret = (ret * 10) + (content_length[i + 1] - '0');
        }
    }

    return ret;
}

static char *hpack_process_path(const unsigned char *path)
{
    char *ret = NULL;

    unsigned char huffman_coded;
    unsigned char value_len;
    unsigned char i;

    char *huffman_decoded = NULL;

    huffman_coded = path[0] & 0x80;
    value_len = path[0] & 0x7F;

    if(huffman_coded)
    {
        huffman_decoded = huffman_decode(path + 1, value_len);

        if(huffman_decoded == NULL)
        {
            return NULL;
        }

        ret = huffman_decoded;
    }
    else
    {
        ret = xmalloc(value_len + 1);
        memcpy(ret, path + 1, value_len);
        ret[value_len] = '\0';
    }

    return ret;
}

char *hpack_resolve_path(const unsigned char *payload)
{
    uint32_t i = 9;
    uint32_t len;

    unsigned char indexed_value;
    unsigned char value_len;

    char *ret;

    len = (payload[0] << 16) | (payload[1] << 8) | payload[2];

    while(i < len + 9)
    {
        if(payload[i] & 0x80)
        {
            indexed_value = (payload[i] & 0x7F);

            switch(indexed_value)
            {
                case 5:
                {
                    ret = xmalloc(sizeof("/index.html"));
                    memcpy(ret, "/index.html", sizeof("/index.html"));
                }
                return ret;

                case 4:
                {
                    ret = hpack_process_path(payload + i + 1);
                }
                return ret;
            }

            ++i;

            continue;
        }

        if(payload[i] & 0x40)
        {
            indexed_value = (payload[i] & 0x3F);

            switch(indexed_value)
            {
                case 4:
                {
                    ret = hpack_process_path(payload + i + 1);
                }
                return ret;

                default:
                {
                    ++i;

                    value_len = (payload[i] & 0x7F);

                    i += value_len + 1;

                    if(indexed_value)
                    {
                        continue;
                    }

                    value_len = (payload[i] & 0x7F);

                    i += value_len + 1;
                }
                break;
            }

            continue;
        }

        if((payload[i] >> 4) == 0 ||
           (payload[i] >> 4) == 1)
        {
            indexed_value = (payload[i] & 0x0F);
            ++i;

            value_len = (payload[i] & 0x7F);

            i += value_len + 1;

            if(indexed_value)
            {
                continue;
            }

            value_len = (payload[i] & 0x7F);

            i += value_len + 1;

            continue;
        }

        logWrite(LOG_TYPE_INFO, "Got unsupported hpack method", 0);

        return NULL;
    }

    return NULL;
}

unsigned int hpack_resolve_content_len(const unsigned char *payload)
{
    uint32_t i = 9;
    uint32_t len;

    unsigned char indexed_value;
    unsigned char value_len;

    len = (payload[0] << 16) | (payload[1] << 8) | payload[2];

    while(i < len + 9)
    {
        if(payload[i] & 0x80)
        {
            ++i;

            continue;
        }

        if(payload[i] & 0x40)
        {
            indexed_value = (payload[i] & 0x3F);
            ++i;

            value_len = (payload[i] & 0x7F);

            switch(indexed_value)
            {
                case 0:
                {
                    i += value_len + 1;
                }
                break;

                case HTTP2_CONTENT_TYPE: return hpack_process_content_length(payload + i);

                default:
                {
                    i += value_len + 1;
                }
                continue;
            }

            value_len = (payload[i] & 0x7F);

            i += value_len + 1;

            continue;
        }

        if((payload[i] >> 4) == 0 ||
           (payload[i] >> 4) == 1)
        {
            indexed_value = (payload[i] & 0x0F);
            ++i;

            value_len = (payload[i] & 0x7F);

            switch(indexed_value)
            {
                case 0:
                {
                    i += value_len + 1;
                }
                break;

                case HTTP2_CONTENT_TYPE: return hpack_process_content_length(payload + i);

                default:
                {
                    i += value_len + 1;
                }
                continue;
            }

            value_len = (payload[i] & 0x7F);

            i += value_len + 1;

            continue;
        }

        logWrite(LOG_TYPE_INFO, "Got unsupported hpack method", 0);

        return 0;
    }

    return 0;
}