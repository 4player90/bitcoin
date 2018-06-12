// Copyright (c) 2017-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifdef ENABLE_SSE41

#include <stdint.h>
#if defined(_MSC_VER)
#include <immintrin.h>
#elif defined(__GNUC__)
#include <x86intrin.h>
#endif

#include <crypto/sha256.h>
#include <crypto/common.h>

namespace sha256_sse41 {

/*
* The implementation in this namespace is a conversion to intrinsics from an original
* YASM implementation by Intel.
*
* Its original copyright text:

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Copyright (c) 2012, Intel Corporation
;
; All rights reserved.
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are
; met:
;
; * Redistributions of source code must retain the above copyright
;   notice, this list of conditions and the following disclaimer.
;
; * Redistributions in binary form must reproduce the above copyright
;   notice, this list of conditions and the following disclaimer in the
;   documentation and/or other materials provided with the
;   distribution.
;
; * Neither the name of the Intel Corporation nor the names of its
;   contributors may be used to endorse or promote products derived from
;   this software without specific prior written permission.
;
;
; THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION "AS IS" AND ANY
; EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
; IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
; PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL CORPORATION OR
; CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
; EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
; PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
; LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
; NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
; SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;
; This code is described in an Intel White-Paper:
; "Fast SHA-256 Implementations on Intel Architecture Processors"
;
; To find it, surf to http://www.intel.com/p/en_US/embedded
; and search for that title.
; The paper is expected to be released roughly at the end of April, 2012
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; This code schedules 1 blocks at a time, with 4 lanes per block
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
*/

namespace {

uint32_t inline __attribute__((always_inline)) Ror(uint32_t x, int val) { return ((x >> val) | (x << (32 - val))); }
__m128i inline __attribute__((always_inline)) Paddd(__m128i x, __m128i y) { return _mm_add_epi32(x, y); }
__m128i inline __attribute__((always_inline)) Pslld(__m128i x, int val) { return _mm_slli_epi32(x, val); }
__m128i inline __attribute__((always_inline)) Psrld(__m128i x, int val) { return _mm_srli_epi32(x, val); }
__m128i inline __attribute__((always_inline)) Psrlq(__m128i x, int val) { return _mm_srli_epi64(x, val); }
__m128i inline __attribute__((always_inline)) Por(__m128i x, __m128i y) { return _mm_or_si128(x, y); }
__m128i inline __attribute__((always_inline)) Pxor(__m128i x, __m128i y) { return _mm_xor_si128(x, y); }

void inline __attribute__((always_inline)) Round(uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d, uint32_t& e, uint32_t& f, uint32_t& g, uint32_t& h, uint32_t w)
{
    uint32_t t1 = (Ror(e, 6) ^ Ror(e, 11) ^ Ror(e, 25)) + (((f ^ g) & e) ^ g) + w;
    uint32_t t2 = (Ror(a, 2) ^ Ror(a, 13) ^ Ror(a, 22)) + (((a | c) & b) | (a & c));
    d += t1;
    h = t1 + t2;
}

void inline __attribute__((always_inline)) QuadRoundSched(uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d, uint32_t& e, uint32_t& f, uint32_t& g, uint32_t& h, __m128i& X0, __m128i X1, __m128i X2, __m128i X3, __m128i W)
{
    alignas(__m128i) uint32_t Ws[4];
    __m128i XTMP0, XTMP1, XTMP2, XTMP3, XTMP4;
    const __m128i SHUF_00BA = _mm_set_epi64x(0xFFFFFFFFFFFFFFFFULL, 0x0b0a090803020100ULL);
    const __m128i SHUF_DC00 = _mm_set_epi64x(0x0b0a090803020100ULL, 0xFFFFFFFFFFFFFFFFULL);

    W = Paddd(W, X0);
    _mm_store_si128((__m128i*)Ws, W);

    Round(a, b, c, d, e, f, g, h, Ws[0]);
    XTMP0 = _mm_alignr_epi8(X3, X2, 4);
    XTMP0 = Paddd(XTMP0, X0);
    XTMP3 = XTMP2 = XTMP1 = _mm_alignr_epi8(X1, X0, 4);
    XTMP2 = Psrld(XTMP2, 7);
    XTMP1 = Por(Pslld(XTMP1, 32 - 7), XTMP2);

    Round(h, a, b, c, d, e, f, g, Ws[1]);
    XTMP2 = XTMP3;
    XTMP4 = XTMP3;
    XTMP3 = Pslld(XTMP3, 32 - 18);
    XTMP2 = Psrld(XTMP2, 18);
    XTMP1 = Pxor(XTMP1, XTMP3);
    XTMP4 = Psrld(XTMP4, 3);
    XTMP1 = Pxor(XTMP1, XTMP2);
    XTMP1 = Pxor(XTMP1, XTMP4);
    XTMP2 = _mm_shuffle_epi32(X3, 0xFA);
    XTMP0 = Paddd(XTMP0, XTMP1);

    Round(g, h, a, b, c, d, e, f, Ws[2]);
    XTMP3 = XTMP2;
    XTMP4 = XTMP2;
    XTMP2 = Psrlq(XTMP2, 17);
    XTMP3 = Psrlq(XTMP3, 19);
    XTMP4 = Psrld(XTMP4, 10);
    XTMP2 = Pxor(XTMP2, XTMP3);
    XTMP4 = Pxor(XTMP4, XTMP2);
    XTMP4 = _mm_shuffle_epi8(XTMP4, SHUF_00BA);
    XTMP0 = Paddd(XTMP0, XTMP4);
    XTMP2 = _mm_shuffle_epi32(XTMP0, 0x50);

    Round(f, g, h, a, b, c, d, e, Ws[3]);
    XTMP3 = XTMP2;
    X0 = XTMP2;
    XTMP2 = Psrlq(XTMP2, 17);
    XTMP3 = Psrlq(XTMP3, 19);
    X0 = Psrld(X0, 10);
    XTMP2 = Pxor(XTMP2, XTMP3);
    X0 = Pxor(X0, XTMP2);
    X0 = _mm_shuffle_epi8(X0, SHUF_DC00);
    X0 = Paddd(X0, XTMP0);
}

void inline __attribute__((always_inline)) QuadRound(uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d, uint32_t& e, uint32_t& f, uint32_t& g, uint32_t& h, __m128i X0, __m128i W)
{
    alignas(__m128i) uint32_t Ws[32];

    X0 = Paddd(X0, W);
    _mm_store_si128((__m128i*)Ws, X0);

    Round(a, b, c, d, e, f, g, h, Ws[0]);
    Round(h, a, b, c, d, e, f, g, Ws[1]);
    Round(g, h, a, b, c, d, e, f, Ws[2]);
    Round(f, g, h, a, b, c, d, e, Ws[3]);
}

__m128i inline __attribute__((always_inline)) KK(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
{
    return _mm_set_epi32(d, c, b, a);
}

}

void Transform(uint32_t* s, const unsigned char* chunk, size_t blocks)
{
    const unsigned char* end = chunk + blocks * 64;
    static const __m128i BYTE_FLIP_MASK = _mm_set_epi64x(0x0c0d0e0f08090a0b, 0x0405060700010203);

    static const __m128i TBL[16] = {
        KK(0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5),
        KK(0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5),
        KK(0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3),
        KK(0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174),
        KK(0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc),
        KK(0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da),
        KK(0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7),
        KK(0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967),
        KK(0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13),
        KK(0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85),
        KK(0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3),
        KK(0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070),
        KK(0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5),
        KK(0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3),
        KK(0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208),
        KK(0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2),
    };

    uint32_t a = s[0], b = s[1], c = s[2], d = s[3], e = s[4], f = s[5], g = s[6], h = s[7];

    __m128i X0, X1, X2, X3;

    while (chunk != end) {
        X0 = _mm_shuffle_epi8(_mm_loadu_si128((const __m128i*)chunk), BYTE_FLIP_MASK);
        X1 = _mm_shuffle_epi8(_mm_loadu_si128((const __m128i*)(chunk + 16)), BYTE_FLIP_MASK);
        X2 = _mm_shuffle_epi8(_mm_loadu_si128((const __m128i*)(chunk + 32)), BYTE_FLIP_MASK);
        X3 = _mm_shuffle_epi8(_mm_loadu_si128((const __m128i*)(chunk + 48)), BYTE_FLIP_MASK);

        QuadRoundSched(a, b, c, d, e, f, g, h, X0, X1, X2, X3, TBL[0]);
        QuadRoundSched(e, f, g, h, a, b, c, d, X1, X2, X3, X0, TBL[1]);
        QuadRoundSched(a, b, c, d, e, f, g, h, X2, X3, X0, X1, TBL[2]);
        QuadRoundSched(e, f, g, h, a, b, c, d, X3, X0, X1, X2, TBL[3]);
        QuadRoundSched(a, b, c, d, e, f, g, h, X0, X1, X2, X3, TBL[4]);
        QuadRoundSched(e, f, g, h, a, b, c, d, X1, X2, X3, X0, TBL[5]);
        QuadRoundSched(a, b, c, d, e, f, g, h, X2, X3, X0, X1, TBL[6]);
        QuadRoundSched(e, f, g, h, a, b, c, d, X3, X0, X1, X2, TBL[7]);
        QuadRoundSched(a, b, c, d, e, f, g, h, X0, X1, X2, X3, TBL[8]);
        QuadRoundSched(e, f, g, h, a, b, c, d, X1, X2, X3, X0, TBL[9]);
        QuadRoundSched(a, b, c, d, e, f, g, h, X2, X3, X0, X1, TBL[10]);
        QuadRoundSched(e, f, g, h, a, b, c, d, X3, X0, X1, X2, TBL[11]);
        QuadRound(a, b, c, d, e, f, g, h, X0, TBL[12]);
        QuadRound(e, f, g, h, a, b, c, d, X1, TBL[13]);
        QuadRound(a, b, c, d, e, f, g, h, X2, TBL[14]);
        QuadRound(e, f, g, h, a, b, c, d, X3, TBL[15]);

        a += s[0]; s[0] = a;
        b += s[1]; s[1] = b;
        c += s[2]; s[2] = c;
        d += s[3]; s[3] = d;
        e += s[4]; s[4] = e;
        f += s[5]; s[5] = f;
        g += s[6]; s[6] = g;
        h += s[7]; s[7] = h;

        chunk += 64;
    }
}
}

namespace sha256d64_sse41 {
namespace {

__m128i inline K(uint32_t x) { return _mm_set1_epi32(x); }

__m128i inline Add(__m128i x, __m128i y) { return _mm_add_epi32(x, y); }
__m128i inline Add(__m128i x, __m128i y, __m128i z) { return Add(Add(x, y), z); }
__m128i inline Add(__m128i x, __m128i y, __m128i z, __m128i w) { return Add(Add(x, y), Add(z, w)); }
__m128i inline Add(__m128i x, __m128i y, __m128i z, __m128i w, __m128i v) { return Add(Add(x, y, z), Add(w, v)); }
__m128i inline Inc(__m128i& x, __m128i y) { x = Add(x, y); return x; }
__m128i inline Inc(__m128i& x, __m128i y, __m128i z) { x = Add(x, y, z); return x; }
__m128i inline Inc(__m128i& x, __m128i y, __m128i z, __m128i w) { x = Add(x, y, z, w); return x; }
__m128i inline Xor(__m128i x, __m128i y) { return _mm_xor_si128(x, y); }
__m128i inline Xor(__m128i x, __m128i y, __m128i z) { return Xor(Xor(x, y), z); }
__m128i inline Or(__m128i x, __m128i y) { return _mm_or_si128(x, y); }
__m128i inline And(__m128i x, __m128i y) { return _mm_and_si128(x, y); }
__m128i inline ShR(__m128i x, int n) { return _mm_srli_epi32(x, n); }
__m128i inline ShL(__m128i x, int n) { return _mm_slli_epi32(x, n); }

__m128i inline Ch(__m128i x, __m128i y, __m128i z) { return Xor(z, And(x, Xor(y, z))); }
__m128i inline Maj(__m128i x, __m128i y, __m128i z) { return Or(And(x, y), And(z, Or(x, y))); }
__m128i inline Sigma0(__m128i x) { return Xor(Or(ShR(x, 2), ShL(x, 30)), Or(ShR(x, 13), ShL(x, 19)), Or(ShR(x, 22), ShL(x, 10))); }
__m128i inline Sigma1(__m128i x) { return Xor(Or(ShR(x, 6), ShL(x, 26)), Or(ShR(x, 11), ShL(x, 21)), Or(ShR(x, 25), ShL(x, 7))); }
__m128i inline sigma0(__m128i x) { return Xor(Or(ShR(x, 7), ShL(x, 25)), Or(ShR(x, 18), ShL(x, 14)), ShR(x, 3)); }
__m128i inline sigma1(__m128i x) { return Xor(Or(ShR(x, 17), ShL(x, 15)), Or(ShR(x, 19), ShL(x, 13)), ShR(x, 10)); }

/** One round of SHA-256. */
void inline __attribute__((always_inline)) Round(__m128i a, __m128i b, __m128i c, __m128i& d, __m128i e, __m128i f, __m128i g, __m128i& h, __m128i k)
{
    __m128i t1 = Add(h, Sigma1(e), Ch(e, f, g), k);
    __m128i t2 = Add(Sigma0(a), Maj(a, b, c));
    d = Add(d, t1);
    h = Add(t1, t2);
}

__m128i inline Read4(const unsigned char* chunk, int offset) {
    __m128i ret = _mm_set_epi32(
        ReadLE32(chunk + 0 + offset),
        ReadLE32(chunk + 64 + offset),
        ReadLE32(chunk + 128 + offset),
        ReadLE32(chunk + 192 + offset)
    );
    return _mm_shuffle_epi8(ret, _mm_set_epi32(0x0C0D0E0FUL, 0x08090A0BUL, 0x04050607UL, 0x00010203UL));
}

void inline Write4(unsigned char* out, int offset, __m128i v) {
    v = _mm_shuffle_epi8(v, _mm_set_epi32(0x0C0D0E0FUL, 0x08090A0BUL, 0x04050607UL, 0x00010203UL));
    WriteLE32(out + 0 + offset, _mm_extract_epi32(v, 3));
    WriteLE32(out + 32 + offset, _mm_extract_epi32(v, 2));
    WriteLE32(out + 64 + offset, _mm_extract_epi32(v, 1));
    WriteLE32(out + 96 + offset, _mm_extract_epi32(v, 0));
}

}

void Transform_4way(unsigned char* out, const unsigned char* in)
{
    // Transform 1
    __m128i a = K(0x6a09e667ul);
    __m128i b = K(0xbb67ae85ul);
    __m128i c = K(0x3c6ef372ul);
    __m128i d = K(0xa54ff53aul);
    __m128i e = K(0x510e527ful);
    __m128i f = K(0x9b05688cul);
    __m128i g = K(0x1f83d9abul);
    __m128i h = K(0x5be0cd19ul);

    __m128i w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10, w11, w12, w13, w14, w15;

    Round(a, b, c, d, e, f, g, h, Add(K(0x428a2f98ul), w0 = Read4(in, 0)));
    Round(h, a, b, c, d, e, f, g, Add(K(0x71374491ul), w1 = Read4(in, 4)));
    Round(g, h, a, b, c, d, e, f, Add(K(0xb5c0fbcful), w2 = Read4(in, 8)));
    Round(f, g, h, a, b, c, d, e, Add(K(0xe9b5dba5ul), w3 = Read4(in, 12)));
    Round(e, f, g, h, a, b, c, d, Add(K(0x3956c25bul), w4 = Read4(in, 16)));
    Round(d, e, f, g, h, a, b, c, Add(K(0x59f111f1ul), w5 = Read4(in, 20)));
    Round(c, d, e, f, g, h, a, b, Add(K(0x923f82a4ul), w6 = Read4(in, 24)));
    Round(b, c, d, e, f, g, h, a, Add(K(0xab1c5ed5ul), w7 = Read4(in, 28)));
    Round(a, b, c, d, e, f, g, h, Add(K(0xd807aa98ul), w8 = Read4(in, 32)));
    Round(h, a, b, c, d, e, f, g, Add(K(0x12835b01ul), w9 = Read4(in, 36)));
    Round(g, h, a, b, c, d, e, f, Add(K(0x243185beul), w10 = Read4(in, 40)));
    Round(f, g, h, a, b, c, d, e, Add(K(0x550c7dc3ul), w11 = Read4(in, 44)));
    Round(e, f, g, h, a, b, c, d, Add(K(0x72be5d74ul), w12 = Read4(in, 48)));
    Round(d, e, f, g, h, a, b, c, Add(K(0x80deb1feul), w13 = Read4(in, 52)));
    Round(c, d, e, f, g, h, a, b, Add(K(0x9bdc06a7ul), w14 = Read4(in, 56)));
    Round(b, c, d, e, f, g, h, a, Add(K(0xc19bf174ul), w15 = Read4(in, 60)));
    Round(a, b, c, d, e, f, g, h, Add(K(0xe49b69c1ul), Inc(w0, sigma1(w14), w9, sigma0(w1))));
    Round(h, a, b, c, d, e, f, g, Add(K(0xefbe4786ul), Inc(w1, sigma1(w15), w10, sigma0(w2))));
    Round(g, h, a, b, c, d, e, f, Add(K(0x0fc19dc6ul), Inc(w2, sigma1(w0), w11, sigma0(w3))));
    Round(f, g, h, a, b, c, d, e, Add(K(0x240ca1ccul), Inc(w3, sigma1(w1), w12, sigma0(w4))));
    Round(e, f, g, h, a, b, c, d, Add(K(0x2de92c6ful), Inc(w4, sigma1(w2), w13, sigma0(w5))));
    Round(d, e, f, g, h, a, b, c, Add(K(0x4a7484aaul), Inc(w5, sigma1(w3), w14, sigma0(w6))));
    Round(c, d, e, f, g, h, a, b, Add(K(0x5cb0a9dcul), Inc(w6, sigma1(w4), w15, sigma0(w7))));
    Round(b, c, d, e, f, g, h, a, Add(K(0x76f988daul), Inc(w7, sigma1(w5), w0, sigma0(w8))));
    Round(a, b, c, d, e, f, g, h, Add(K(0x983e5152ul), Inc(w8, sigma1(w6), w1, sigma0(w9))));
    Round(h, a, b, c, d, e, f, g, Add(K(0xa831c66dul), Inc(w9, sigma1(w7), w2, sigma0(w10))));
    Round(g, h, a, b, c, d, e, f, Add(K(0xb00327c8ul), Inc(w10, sigma1(w8), w3, sigma0(w11))));
    Round(f, g, h, a, b, c, d, e, Add(K(0xbf597fc7ul), Inc(w11, sigma1(w9), w4, sigma0(w12))));
    Round(e, f, g, h, a, b, c, d, Add(K(0xc6e00bf3ul), Inc(w12, sigma1(w10), w5, sigma0(w13))));
    Round(d, e, f, g, h, a, b, c, Add(K(0xd5a79147ul), Inc(w13, sigma1(w11), w6, sigma0(w14))));
    Round(c, d, e, f, g, h, a, b, Add(K(0x06ca6351ul), Inc(w14, sigma1(w12), w7, sigma0(w15))));
    Round(b, c, d, e, f, g, h, a, Add(K(0x14292967ul), Inc(w15, sigma1(w13), w8, sigma0(w0))));
    Round(a, b, c, d, e, f, g, h, Add(K(0x27b70a85ul), Inc(w0, sigma1(w14), w9, sigma0(w1))));
    Round(h, a, b, c, d, e, f, g, Add(K(0x2e1b2138ul), Inc(w1, sigma1(w15), w10, sigma0(w2))));
    Round(g, h, a, b, c, d, e, f, Add(K(0x4d2c6dfcul), Inc(w2, sigma1(w0), w11, sigma0(w3))));
    Round(f, g, h, a, b, c, d, e, Add(K(0x53380d13ul), Inc(w3, sigma1(w1), w12, sigma0(w4))));
    Round(e, f, g, h, a, b, c, d, Add(K(0x650a7354ul), Inc(w4, sigma1(w2), w13, sigma0(w5))));
    Round(d, e, f, g, h, a, b, c, Add(K(0x766a0abbul), Inc(w5, sigma1(w3), w14, sigma0(w6))));
    Round(c, d, e, f, g, h, a, b, Add(K(0x81c2c92eul), Inc(w6, sigma1(w4), w15, sigma0(w7))));
    Round(b, c, d, e, f, g, h, a, Add(K(0x92722c85ul), Inc(w7, sigma1(w5), w0, sigma0(w8))));
    Round(a, b, c, d, e, f, g, h, Add(K(0xa2bfe8a1ul), Inc(w8, sigma1(w6), w1, sigma0(w9))));
    Round(h, a, b, c, d, e, f, g, Add(K(0xa81a664bul), Inc(w9, sigma1(w7), w2, sigma0(w10))));
    Round(g, h, a, b, c, d, e, f, Add(K(0xc24b8b70ul), Inc(w10, sigma1(w8), w3, sigma0(w11))));
    Round(f, g, h, a, b, c, d, e, Add(K(0xc76c51a3ul), Inc(w11, sigma1(w9), w4, sigma0(w12))));
    Round(e, f, g, h, a, b, c, d, Add(K(0xd192e819ul), Inc(w12, sigma1(w10), w5, sigma0(w13))));
    Round(d, e, f, g, h, a, b, c, Add(K(0xd6990624ul), Inc(w13, sigma1(w11), w6, sigma0(w14))));
    Round(c, d, e, f, g, h, a, b, Add(K(0xf40e3585ul), Inc(w14, sigma1(w12), w7, sigma0(w15))));
    Round(b, c, d, e, f, g, h, a, Add(K(0x106aa070ul), Inc(w15, sigma1(w13), w8, sigma0(w0))));
    Round(a, b, c, d, e, f, g, h, Add(K(0x19a4c116ul), Inc(w0, sigma1(w14), w9, sigma0(w1))));
    Round(h, a, b, c, d, e, f, g, Add(K(0x1e376c08ul), Inc(w1, sigma1(w15), w10, sigma0(w2))));
    Round(g, h, a, b, c, d, e, f, Add(K(0x2748774cul), Inc(w2, sigma1(w0), w11, sigma0(w3))));
    Round(f, g, h, a, b, c, d, e, Add(K(0x34b0bcb5ul), Inc(w3, sigma1(w1), w12, sigma0(w4))));
    Round(e, f, g, h, a, b, c, d, Add(K(0x391c0cb3ul), Inc(w4, sigma1(w2), w13, sigma0(w5))));
    Round(d, e, f, g, h, a, b, c, Add(K(0x4ed8aa4aul), Inc(w5, sigma1(w3), w14, sigma0(w6))));
    Round(c, d, e, f, g, h, a, b, Add(K(0x5b9cca4ful), Inc(w6, sigma1(w4), w15, sigma0(w7))));
    Round(b, c, d, e, f, g, h, a, Add(K(0x682e6ff3ul), Inc(w7, sigma1(w5), w0, sigma0(w8))));
    Round(a, b, c, d, e, f, g, h, Add(K(0x748f82eeul), Inc(w8, sigma1(w6), w1, sigma0(w9))));
    Round(h, a, b, c, d, e, f, g, Add(K(0x78a5636ful), Inc(w9, sigma1(w7), w2, sigma0(w10))));
    Round(g, h, a, b, c, d, e, f, Add(K(0x84c87814ul), Inc(w10, sigma1(w8), w3, sigma0(w11))));
    Round(f, g, h, a, b, c, d, e, Add(K(0x8cc70208ul), Inc(w11, sigma1(w9), w4, sigma0(w12))));
    Round(e, f, g, h, a, b, c, d, Add(K(0x90befffaul), Inc(w12, sigma1(w10), w5, sigma0(w13))));
    Round(d, e, f, g, h, a, b, c, Add(K(0xa4506cebul), Inc(w13, sigma1(w11), w6, sigma0(w14))));
    Round(c, d, e, f, g, h, a, b, Add(K(0xbef9a3f7ul), Inc(w14, sigma1(w12), w7, sigma0(w15))));
    Round(b, c, d, e, f, g, h, a, Add(K(0xc67178f2ul), Inc(w15, sigma1(w13), w8, sigma0(w0))));

    a = Add(a, K(0x6a09e667ul));
    b = Add(b, K(0xbb67ae85ul));
    c = Add(c, K(0x3c6ef372ul));
    d = Add(d, K(0xa54ff53aul));
    e = Add(e, K(0x510e527ful));
    f = Add(f, K(0x9b05688cul));
    g = Add(g, K(0x1f83d9abul));
    h = Add(h, K(0x5be0cd19ul));

    __m128i t0 = a, t1 = b, t2 = c, t3 = d, t4 = e, t5 = f, t6 = g, t7 = h;

    // Transform 2
    Round(a, b, c, d, e, f, g, h, K(0xc28a2f98ul));
    Round(h, a, b, c, d, e, f, g, K(0x71374491ul));
    Round(g, h, a, b, c, d, e, f, K(0xb5c0fbcful));
    Round(f, g, h, a, b, c, d, e, K(0xe9b5dba5ul));
    Round(e, f, g, h, a, b, c, d, K(0x3956c25bul));
    Round(d, e, f, g, h, a, b, c, K(0x59f111f1ul));
    Round(c, d, e, f, g, h, a, b, K(0x923f82a4ul));
    Round(b, c, d, e, f, g, h, a, K(0xab1c5ed5ul));
    Round(a, b, c, d, e, f, g, h, K(0xd807aa98ul));
    Round(h, a, b, c, d, e, f, g, K(0x12835b01ul));
    Round(g, h, a, b, c, d, e, f, K(0x243185beul));
    Round(f, g, h, a, b, c, d, e, K(0x550c7dc3ul));
    Round(e, f, g, h, a, b, c, d, K(0x72be5d74ul));
    Round(d, e, f, g, h, a, b, c, K(0x80deb1feul));
    Round(c, d, e, f, g, h, a, b, K(0x9bdc06a7ul));
    Round(b, c, d, e, f, g, h, a, K(0xc19bf374ul));
    Round(a, b, c, d, e, f, g, h, K(0x649b69c1ul));
    Round(h, a, b, c, d, e, f, g, K(0xf0fe4786ul));
    Round(g, h, a, b, c, d, e, f, K(0x0fe1edc6ul));
    Round(f, g, h, a, b, c, d, e, K(0x240cf254ul));
    Round(e, f, g, h, a, b, c, d, K(0x4fe9346ful));
    Round(d, e, f, g, h, a, b, c, K(0x6cc984beul));
    Round(c, d, e, f, g, h, a, b, K(0x61b9411eul));
    Round(b, c, d, e, f, g, h, a, K(0x16f988faul));
    Round(a, b, c, d, e, f, g, h, K(0xf2c65152ul));
    Round(h, a, b, c, d, e, f, g, K(0xa88e5a6dul));
    Round(g, h, a, b, c, d, e, f, K(0xb019fc65ul));
    Round(f, g, h, a, b, c, d, e, K(0xb9d99ec7ul));
    Round(e, f, g, h, a, b, c, d, K(0x9a1231c3ul));
    Round(d, e, f, g, h, a, b, c, K(0xe70eeaa0ul));
    Round(c, d, e, f, g, h, a, b, K(0xfdb1232bul));
    Round(b, c, d, e, f, g, h, a, K(0xc7353eb0ul));
    Round(a, b, c, d, e, f, g, h, K(0x3069bad5ul));
    Round(h, a, b, c, d, e, f, g, K(0xcb976d5ful));
    Round(g, h, a, b, c, d, e, f, K(0x5a0f118ful));
    Round(f, g, h, a, b, c, d, e, K(0xdc1eeefdul));
    Round(e, f, g, h, a, b, c, d, K(0x0a35b689ul));
    Round(d, e, f, g, h, a, b, c, K(0xde0b7a04ul));
    Round(c, d, e, f, g, h, a, b, K(0x58f4ca9dul));
    Round(b, c, d, e, f, g, h, a, K(0xe15d5b16ul));
    Round(a, b, c, d, e, f, g, h, K(0x007f3e86ul));
    Round(h, a, b, c, d, e, f, g, K(0x37088980ul));
    Round(g, h, a, b, c, d, e, f, K(0xa507ea32ul));
    Round(f, g, h, a, b, c, d, e, K(0x6fab9537ul));
    Round(e, f, g, h, a, b, c, d, K(0x17406110ul));
    Round(d, e, f, g, h, a, b, c, K(0x0d8cd6f1ul));
    Round(c, d, e, f, g, h, a, b, K(0xcdaa3b6dul));
    Round(b, c, d, e, f, g, h, a, K(0xc0bbbe37ul));
    Round(a, b, c, d, e, f, g, h, K(0x83613bdaul));
    Round(h, a, b, c, d, e, f, g, K(0xdb48a363ul));
    Round(g, h, a, b, c, d, e, f, K(0x0b02e931ul));
    Round(f, g, h, a, b, c, d, e, K(0x6fd15ca7ul));
    Round(e, f, g, h, a, b, c, d, K(0x521afacaul));
    Round(d, e, f, g, h, a, b, c, K(0x31338431ul));
    Round(c, d, e, f, g, h, a, b, K(0x6ed41a95ul));
    Round(b, c, d, e, f, g, h, a, K(0x6d437890ul));
    Round(a, b, c, d, e, f, g, h, K(0xc39c91f2ul));
    Round(h, a, b, c, d, e, f, g, K(0x9eccabbdul));
    Round(g, h, a, b, c, d, e, f, K(0xb5c9a0e6ul));
    Round(f, g, h, a, b, c, d, e, K(0x532fb63cul));
    Round(e, f, g, h, a, b, c, d, K(0xd2c741c6ul));
    Round(d, e, f, g, h, a, b, c, K(0x07237ea3ul));
    Round(c, d, e, f, g, h, a, b, K(0xa4954b68ul));
    Round(b, c, d, e, f, g, h, a, K(0x4c191d76ul));

    w0 = Add(t0, a);
    w1 = Add(t1, b);
    w2 = Add(t2, c);
    w3 = Add(t3, d);
    w4 = Add(t4, e);
    w5 = Add(t5, f);
    w6 = Add(t6, g);
    w7 = Add(t7, h);

    // Transform 3
    a = K(0x6a09e667ul);
    b = K(0xbb67ae85ul);
    c = K(0x3c6ef372ul);
    d = K(0xa54ff53aul);
    e = K(0x510e527ful);
    f = K(0x9b05688cul);
    g = K(0x1f83d9abul);
    h = K(0x5be0cd19ul);

    Round(a, b, c, d, e, f, g, h, Add(K(0x428a2f98ul), w0));
    Round(h, a, b, c, d, e, f, g, Add(K(0x71374491ul), w1));
    Round(g, h, a, b, c, d, e, f, Add(K(0xb5c0fbcful), w2));
    Round(f, g, h, a, b, c, d, e, Add(K(0xe9b5dba5ul), w3));
    Round(e, f, g, h, a, b, c, d, Add(K(0x3956c25bul), w4));
    Round(d, e, f, g, h, a, b, c, Add(K(0x59f111f1ul), w5));
    Round(c, d, e, f, g, h, a, b, Add(K(0x923f82a4ul), w6));
    Round(b, c, d, e, f, g, h, a, Add(K(0xab1c5ed5ul), w7));
    Round(a, b, c, d, e, f, g, h, K(0x5807aa98ul));
    Round(h, a, b, c, d, e, f, g, K(0x12835b01ul));
    Round(g, h, a, b, c, d, e, f, K(0x243185beul));
    Round(f, g, h, a, b, c, d, e, K(0x550c7dc3ul));
    Round(e, f, g, h, a, b, c, d, K(0x72be5d74ul));
    Round(d, e, f, g, h, a, b, c, K(0x80deb1feul));
    Round(c, d, e, f, g, h, a, b, K(0x9bdc06a7ul));
    Round(b, c, d, e, f, g, h, a, K(0xc19bf274ul));
    Round(a, b, c, d, e, f, g, h, Add(K(0xe49b69c1ul), Inc(w0, sigma0(w1))));
    Round(h, a, b, c, d, e, f, g, Add(K(0xefbe4786ul), Inc(w1, K(0xa00000ul), sigma0(w2))));
    Round(g, h, a, b, c, d, e, f, Add(K(0x0fc19dc6ul), Inc(w2, sigma1(w0), sigma0(w3))));
    Round(f, g, h, a, b, c, d, e, Add(K(0x240ca1ccul), Inc(w3, sigma1(w1), sigma0(w4))));
    Round(e, f, g, h, a, b, c, d, Add(K(0x2de92c6ful), Inc(w4, sigma1(w2), sigma0(w5))));
    Round(d, e, f, g, h, a, b, c, Add(K(0x4a7484aaul), Inc(w5, sigma1(w3), sigma0(w6))));
    Round(c, d, e, f, g, h, a, b, Add(K(0x5cb0a9dcul), Inc(w6, sigma1(w4), K(0x100ul), sigma0(w7))));
    Round(b, c, d, e, f, g, h, a, Add(K(0x76f988daul), Inc(w7, sigma1(w5), w0, K(0x11002000ul))));
    Round(a, b, c, d, e, f, g, h, Add(K(0x983e5152ul), w8 = Add(K(0x80000000ul), sigma1(w6), w1)));
    Round(h, a, b, c, d, e, f, g, Add(K(0xa831c66dul), w9 = Add(sigma1(w7), w2)));
    Round(g, h, a, b, c, d, e, f, Add(K(0xb00327c8ul), w10 = Add(sigma1(w8), w3)));
    Round(f, g, h, a, b, c, d, e, Add(K(0xbf597fc7ul), w11 = Add(sigma1(w9), w4)));
    Round(e, f, g, h, a, b, c, d, Add(K(0xc6e00bf3ul), w12 = Add(sigma1(w10), w5)));
    Round(d, e, f, g, h, a, b, c, Add(K(0xd5a79147ul), w13 = Add(sigma1(w11), w6)));
    Round(c, d, e, f, g, h, a, b, Add(K(0x06ca6351ul), w14 = Add(sigma1(w12), w7, K(0x400022ul))));
    Round(b, c, d, e, f, g, h, a, Add(K(0x14292967ul), w15 = Add(K(0x100ul), sigma1(w13), w8, sigma0(w0))));
    Round(a, b, c, d, e, f, g, h, Add(K(0x27b70a85ul), Inc(w0, sigma1(w14), w9, sigma0(w1))));
    Round(h, a, b, c, d, e, f, g, Add(K(0x2e1b2138ul), Inc(w1, sigma1(w15), w10, sigma0(w2))));
    Round(g, h, a, b, c, d, e, f, Add(K(0x4d2c6dfcul), Inc(w2, sigma1(w0), w11, sigma0(w3))));
    Round(f, g, h, a, b, c, d, e, Add(K(0x53380d13ul), Inc(w3, sigma1(w1), w12, sigma0(w4))));
    Round(e, f, g, h, a, b, c, d, Add(K(0x650a7354ul), Inc(w4, sigma1(w2), w13, sigma0(w5))));
    Round(d, e, f, g, h, a, b, c, Add(K(0x766a0abbul), Inc(w5, sigma1(w3), w14, sigma0(w6))));
    Round(c, d, e, f, g, h, a, b, Add(K(0x81c2c92eul), Inc(w6, sigma1(w4), w15, sigma0(w7))));
    Round(b, c, d, e, f, g, h, a, Add(K(0x92722c85ul), Inc(w7, sigma1(w5), w0, sigma0(w8))));
    Round(a, b, c, d, e, f, g, h, Add(K(0xa2bfe8a1ul), Inc(w8, sigma1(w6), w1, sigma0(w9))));
    Round(h, a, b, c, d, e, f, g, Add(K(0xa81a664bul), Inc(w9, sigma1(w7), w2, sigma0(w10))));
    Round(g, h, a, b, c, d, e, f, Add(K(0xc24b8b70ul), Inc(w10, sigma1(w8), w3, sigma0(w11))));
    Round(f, g, h, a, b, c, d, e, Add(K(0xc76c51a3ul), Inc(w11, sigma1(w9), w4, sigma0(w12))));
    Round(e, f, g, h, a, b, c, d, Add(K(0xd192e819ul), Inc(w12, sigma1(w10), w5, sigma0(w13))));
    Round(d, e, f, g, h, a, b, c, Add(K(0xd6990624ul), Inc(w13, sigma1(w11), w6, sigma0(w14))));
    Round(c, d, e, f, g, h, a, b, Add(K(0xf40e3585ul), Inc(w14, sigma1(w12), w7, sigma0(w15))));
    Round(b, c, d, e, f, g, h, a, Add(K(0x106aa070ul), Inc(w15, sigma1(w13), w8, sigma0(w0))));
    Round(a, b, c, d, e, f, g, h, Add(K(0x19a4c116ul), Inc(w0, sigma1(w14), w9, sigma0(w1))));
    Round(h, a, b, c, d, e, f, g, Add(K(0x1e376c08ul), Inc(w1, sigma1(w15), w10, sigma0(w2))));
    Round(g, h, a, b, c, d, e, f, Add(K(0x2748774cul), Inc(w2, sigma1(w0), w11, sigma0(w3))));
    Round(f, g, h, a, b, c, d, e, Add(K(0x34b0bcb5ul), Inc(w3, sigma1(w1), w12, sigma0(w4))));
    Round(e, f, g, h, a, b, c, d, Add(K(0x391c0cb3ul), Inc(w4, sigma1(w2), w13, sigma0(w5))));
    Round(d, e, f, g, h, a, b, c, Add(K(0x4ed8aa4aul), Inc(w5, sigma1(w3), w14, sigma0(w6))));
    Round(c, d, e, f, g, h, a, b, Add(K(0x5b9cca4ful), Inc(w6, sigma1(w4), w15, sigma0(w7))));
    Round(b, c, d, e, f, g, h, a, Add(K(0x682e6ff3ul), Inc(w7, sigma1(w5), w0, sigma0(w8))));
    Round(a, b, c, d, e, f, g, h, Add(K(0x748f82eeul), Inc(w8, sigma1(w6), w1, sigma0(w9))));
    Round(h, a, b, c, d, e, f, g, Add(K(0x78a5636ful), Inc(w9, sigma1(w7), w2, sigma0(w10))));
    Round(g, h, a, b, c, d, e, f, Add(K(0x84c87814ul), Inc(w10, sigma1(w8), w3, sigma0(w11))));
    Round(f, g, h, a, b, c, d, e, Add(K(0x8cc70208ul), Inc(w11, sigma1(w9), w4, sigma0(w12))));
    Round(e, f, g, h, a, b, c, d, Add(K(0x90befffaul), Inc(w12, sigma1(w10), w5, sigma0(w13))));
    Round(d, e, f, g, h, a, b, c, Add(K(0xa4506cebul), Inc(w13, sigma1(w11), w6, sigma0(w14))));
    Round(c, d, e, f, g, h, a, b, Add(K(0xbef9a3f7ul), w14, sigma1(w12), w7, sigma0(w15)));
    Round(b, c, d, e, f, g, h, a, Add(K(0xc67178f2ul), w15, sigma1(w13), w8, sigma0(w0)));

    // Output
    Write4(out, 0, Add(a, K(0x6a09e667ul)));
    Write4(out, 4, Add(b, K(0xbb67ae85ul)));
    Write4(out, 8, Add(c, K(0x3c6ef372ul)));
    Write4(out, 12, Add(d, K(0xa54ff53aul)));
    Write4(out, 16, Add(e, K(0x510e527ful)));
    Write4(out, 20, Add(f, K(0x9b05688cul)));
    Write4(out, 24, Add(g, K(0x1f83d9abul)));
    Write4(out, 28, Add(h, K(0x5be0cd19ul)));
}

}

#endif
