/*
 * Copyright © 2019-2020 The Crust Firmware Authors.
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-only
 */

#include <counter.h>
#include <delay.h>
#include <mmio.h>
#include <stdint.h>
#include <watchdog/sunxi-twd.h>
#include <platform/devices.h>
#include <platform/prcm.h>
#include <platform/time.h>

#include "ccu.h"

#define PLL_CTRL_REG1_MASK (PLL_CTRL_REG1_KEY_FIELD | \
	                    PLL_CTRL_REG1_CRYSTAL_EN | \
	                    PLL_CTRL_REG1_LDO_EN)

/* Persist this var as r_ccu_init() may not be called after an exception. */
static uint32_t osc16m_rate = 16000000U;

DEFINE_FIXED_RATE(r_ccu_common_get_osc16m_rate, osc16m_rate)

/**
 * Write a value to the lockable bits in PLL_CTRL_REG1.
 */
static void
r_ccu_common_update_pll_ctrl_reg1(uint32_t new)
{
	uint32_t val = mmio_read_32(PLL_CTRL_REG1) & ~PLL_CTRL_REG1_MASK;

	/* Unlock if locked, otherwise write value. */
	mmio_write_32(PLL_CTRL_REG1, val | new | PLL_CTRL_REG1_KEY);
	/* Write value if just unlocked, otherwise write same; lock. */
	mmio_write_32(PLL_CTRL_REG1, val | new);
}

void
r_ccu_common_suspend(void)
{
	if (!CONFIG(SUSPEND_OSC24M))
		return;

	r_ccu_common_update_pll_ctrl_reg1(PLL_CTRL_REG1_LDO_EN);
	udelay(1);
	r_ccu_common_update_pll_ctrl_reg1(0);
	mmio_set_32(VDD_SYS_PWROFF_GATING_REG,
	            CONFIG(GATE_VDD_SYS) * VDD_CPUS_GATING | VCC_PLL_GATING);
}

void WEAK ATTRIBUTE(alias("r_ccu_common_suspend"))
r_ccu_suspend(void);

void
r_ccu_common_resume(void)
{
	if (!CONFIG(SUSPEND_OSC24M))
		return;

	mmio_clr_32(VDD_SYS_PWROFF_GATING_REG,
	            CONFIG(GATE_VDD_SYS) * VDD_CPUS_GATING | VCC_PLL_GATING);
	r_ccu_common_update_pll_ctrl_reg1(PLL_CTRL_REG1_LDO_EN);
	udelay(2000);
	r_ccu_common_update_pll_ctrl_reg1(PLL_CTRL_REG1_CRYSTAL_EN |
	                                  PLL_CTRL_REG1_LDO_EN);
}

void WEAK ATTRIBUTE(alias("r_ccu_common_resume"))
r_ccu_resume(void);

void
r_ccu_common_init(void)
{
	uint32_t after, before, end, now;

	/* Cycle until the interval will not span a counter wraparound. */
	do {
		before = counter_read();
		barrier();
		now = r_twd_counter_read();
		end = now + (REFCLK_HZ >> 9);
	} while (end < now);

	/* Cycle until the end of the interval. */
	do {
		after = counter_read();
		/* Ensure the counters are read in a consistent order. */
		barrier();
		now = r_twd_counter_read();
	} while (now < end);

	/*
	 * Convert the number of OSC16M cycles in 1/512 second to Hz. 512 is
	 * chosen because it is the largest power-of-two factor of 24MHz, the
	 * reference clock frequency.
	 */
	osc16m_rate = (after - before) << 9;
}
