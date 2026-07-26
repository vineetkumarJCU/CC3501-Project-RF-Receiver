#ifndef FIXED_LOG10_TABLE_H
#define FIXED_LOG10_TABLE_H

#include <stdint.h>

/*
 * Q20.12 values of 20 * log10(1 + index / 256), including the endpoint
 * at index 256. The endpoint lets the lookup interpolate each of the 256
 * normalized-mantissa intervals without a special case.
 */
#define FIXED_LOG10_DB_FRACTION_BITS 12U
#define FIXED_LOG10_LUT_INDEX_BITS    8U
#define FIXED_LOG10_LUT_SIZE          ((1U << FIXED_LOG10_LUT_INDEX_BITS) + 1U)
#define FIXED_LOG10_20LOG2_Q12        24660L

static const uint16_t fixed_log10_20log_mantissa_q12[FIXED_LOG10_LUT_SIZE] = {
        0,   139,   277,   414,   552,   688,   824,   960,
     1095,  1229,  1363,  1497,  1630,  1762,  1894,  2026,
     2157,  2287,  2418,  2547,  2676,  2805,  2933,  3061,
     3188,  3315,  3441,  3567,  3693,  3818,  3942,  4067,
     4190,  4314,  4437,  4559,  4681,  4803,  4924,  5045,
     5165,  5285,  5405,  5524,  5643,  5761,  5879,  5997,
     6114,  6231,  6347,  6463,  6579,  6694,  6809,  6924,
     7038,  7152,  7265,  7379,  7491,  7604,  7716,  7828,
     7939,  8050,  8161,  8271,  8381,  8490,  8600,  8709,
     8817,  8926,  9034,  9141,  9249,  9356,  9462,  9569,
     9675,  9780,  9886,  9991, 10096, 10200, 10304, 10408,
    10512, 10615, 10718, 10821, 10923, 11025, 11127, 11229,
    11330, 11431, 11531, 11632, 11732, 11832, 11931, 12030,
    12129, 12228, 12326, 12425, 12522, 12620, 12717, 12814,
    12911, 13008, 13104, 13200, 13296, 13391, 13487, 13582,
    13676, 13771, 13865, 13959, 14053, 14146, 14240, 14333,
    14425, 14518, 14610, 14702, 14794, 14886, 14977, 15068,
    15159, 15250, 15340, 15430, 15520, 15610, 15699, 15789,
    15878, 15967, 16055, 16144, 16232, 16320, 16407, 16495,
    16582, 16669, 16756, 16843, 16929, 17016, 17102, 17187,
    17273, 17359, 17444, 17529, 17614, 17698, 17783, 17867,
    17951, 18035, 18118, 18202, 18285, 18368, 18451, 18533,
    18616, 18698, 18780, 18862, 18944, 19025, 19107, 19188,
    19269, 19349, 19430, 19510, 19591, 19671, 19750, 19830,
    19910, 19989, 20068, 20147, 20226, 20305, 20383, 20461,
    20539, 20617, 20695, 20773, 20850, 20927, 21004, 21081,
    21158, 21235, 21311, 21387, 21464, 21539, 21615, 21691,
    21766, 21842, 21917, 21992, 22067, 22141, 22216, 22290,
    22364, 22438, 22512, 22586, 22660, 22733, 22806, 22879,
    22952, 23025, 23098, 23170, 23243, 23315, 23387, 23459,
    23531, 23602, 23674, 23745, 23817, 23888, 23959, 24029,
    24100, 24171, 24241, 24311, 24381, 24451, 24521, 24591,
    24660,
};

static inline uint32_t fixed_log10_floor_log2_u32(uint32_t value)
{
#if defined(__GNUC__) || defined(__clang__)
    return 31U - (uint32_t)__builtin_clz(value);
#else
    uint32_t exponent = 0U;
    while ((value >>= 1U) != 0U) exponent++;
    return exponent;
#endif
}

/* Return 20 * log10(value) in Q20.12. The caller must pass value > 0. */
static inline int32_t fixed_log10_20log_u32_q12(uint32_t value)
{
    const uint32_t exponent = fixed_log10_floor_log2_u32(value);
    const uint32_t normalized_q16 = exponent >= 16U
                                              ? value >> (exponent - 16U)
                                              : value << (16U - exponent);
    const uint32_t mantissa_offset = normalized_q16 - (1U << 16U);
    const uint32_t table_index = mantissa_offset >> 8U;
    const uint32_t fraction_q8 = mantissa_offset & 0xffU;
    const uint32_t lower = fixed_log10_20log_mantissa_q12[table_index];
    const uint32_t upper = fixed_log10_20log_mantissa_q12[table_index + 1U];
    const uint32_t interpolated =
        lower + (((upper - lower) * fraction_q8 + 128U) >> 8U);

    return (int32_t)(exponent * (uint32_t)FIXED_LOG10_20LOG2_Q12) +
           (int32_t)interpolated;
}

#endif /* FIXED_LOG10_TABLE_H */
