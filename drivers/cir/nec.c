/*
 * Copyright © 2020-2021 The Crust Firmware Authors.
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-only
 */

#include <debug.h>
#include <stdbool.h>
#include <util.h>

#include "rc6.h"

#define NUM_DATA_BITS   32

#define NEC_UNIT 563UL

#define NEC_UNITS_TO_CLKS(num) US_TO_CLKS((num) * NEC_UNIT)

#if CONFIG(CIR_NECX)
#define NEC_LEAD_P   NEC_UNITS_TO_CLKS(8)
#else
#define NEC_LEAD_P   NEC_UNITS_TO_CLKS(16)
#endif
#define NEC_LEAD_S   NEC_UNITS_TO_CLKS(8)
#define NEC_DATA_P   NEC_UNITS_TO_CLKS(1)
#define NEC_DATA_S_0 NEC_UNITS_TO_CLKS(1)
#define NEC_DATA_S_1 NEC_UNITS_TO_CLKS(3)

#define NEC_HALF_MARGIN   (NEC_UNITS_TO_CLKS(1) / 2)
#define NEC_SINGLE_MARGIN NEC_UNITS_TO_CLKS(1)
#define NEC_DOUBLE_MARGIN NEC_UNITS_TO_CLKS(2)

enum {
	NEC_IDLE,
	NEC_HEAD_S,
	NEC_PULSE,
	NEC_DATA,
	NEC_STATES
};

static const uint8_t nec_pulse_states[NEC_STATES] = {
	[NEC_IDLE] = 1,
	[NEC_HEAD_S] = 0,
	[NEC_PULSE] = 1,
	[NEC_DATA] = 0,
};

uint32_t
rc6_decode(struct dec_rtx *ctx)
{
	uint32_t ret = 0;

	if (nec_pulse_states[ctx->state] == ctx->pulse) {
		ctx->counter += ctx->width;
		ctx->width = 0;
		return 0;
	}

	switch (ctx->state) {
	case NEC_IDLE:
		if (EQ_MARGIN(ctx->counter, NEC_LEAD_P, NEC_DOUBLE_MARGIN))
			ctx->state = NEC_HEAD_S;
		else
			ctx->width = 0;
		break;
	case NEC_HEAD_S:
		if (EQ_MARGIN(ctx->counter, NEC_LEAD_S, NEC_SINGLE_MARGIN)) {
			ctx->bits = 0;
			ctx->buffer = 0;
			ctx->state = NEC_PULSE;
		} else {
			ctx->state = NEC_IDLE;
		}
		break;
	case NEC_PULSE:
		ctx->state = NEC_IDLE;
		if (!EQ_MARGIN(ctx->counter, NEC_DATA_P, NEC_HALF_MARGIN))
			break;
		if (ctx->bits == NUM_DATA_BITS) {
			/* it would be nice to check if inverted values match */
			if (CONFIG(CIR_NECX))
				ret = ((ctx->buffer << 16) & GENMASK(23, 16)) |
				      (ctx->buffer & GENMASK(15, 8)) |
				      ((ctx->buffer >> 16) & GENMASK(7, 0));
			else
				ret = ((ctx->buffer << 8) & GENMASK(15, 8)) |
				      ((ctx->buffer >> 16) & GENMASK(7, 0));
			debug("NEC code %08x", ret);
		} else {
			ctx->state = NEC_DATA;
		}
		break;
	case NEC_DATA:
		/* NEC is LSB first */
		ctx->buffer >>= 1;
		ctx->bits++;
		ctx->state = NEC_PULSE;
		if (EQ_MARGIN(ctx->counter, NEC_DATA_S_1, NEC_HALF_MARGIN))
			ctx->buffer |= BIT(31);
		else if (!EQ_MARGIN(ctx->counter, NEC_DATA_S_0, NEC_HALF_MARGIN))
			ctx->state = NEC_IDLE;
		break;
	default:
		unreachable();
	}

	ctx->counter = 0;

	return ret;
}
