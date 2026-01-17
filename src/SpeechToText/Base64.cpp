#include "Base64.h"

static const char* B64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64_encode(const uint8_t* data, size_t len)
{
    std::string out;
    out.reserve(((len + 2) / 3) * 4);

    size_t i = 0;
    while (i < len)
    {
        const uint32_t a = i < len ? data[i++] : 0;
        const uint32_t b = i < len ? data[i++] : 0;
        const uint32_t c = i < len ? data[i++] : 0;

        const uint32_t triple = (a << 16) | (b << 8) | c;

        out.push_back(B64[(triple >> 18) & 0x3F]);
        out.push_back(B64[(triple >> 12) & 0x3F]);
        out.push_back(B64[(triple >> 6) & 0x3F]);
        out.push_back(B64[triple & 0x3F]);
    }

    const size_t mod = len % 3;
    if (mod)
    {
        out[out.size() - 1] = '=';
        if (mod == 1) out[out.size() - 2] = '=';
    }

    return out;
}
