/*
 * Copyright © 2020-2021 The Crust Firmware Authors.
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-only
 */

#ifndef RC6_PRIVATE_H
#define RC6_PRIVATE_H

#include <stdint.h>

#if CONFIG(CIR_USE_OSC24M)
/* parent clock is predivided by 192 */
#define CIR_CLK_RATE 125000UL
#else
#define CIR_CLK_RATE 32768UL
#endif

#define US_TO_CLKS(num) (((num) * CIR_CLK_RATE) / 1000000UL)

#define EQ_MARGIN(val, time, margin) \
	(((time) - (margin)) < (val) && (val) < ((time) + (margin)))

struct dec_rtx {
	uint32_t buffer;
	uint32_t counter;
	uint8_t  bits;
	uint8_t  state;
	uint8_t  pulse;
	int8_t   width;
};

/**
 * Decode an RC6 pulse sequence.
 *
 * The pulse flag and width must be valid each time this function is called.
 * Each call will decode a single pulse, so decoding a complete scancode
 * requires many calls. All but the last in the sequence will return zero.
 *
 * If an error occurs, decoding restarts, but the error is not reported.
 *
 * @return A successfully decoded scancode, or zero.
 */
uint32_t rc6_decode(struct dec_rtx *ctx);

#endif /* RC6_PRIVATE_H */
