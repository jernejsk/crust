/*
 * Copyright © 2017-2020 The Crust Firmware Authors.
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-only
 */

#include <device.h>
#include <error.h>
#include <regmap.h>
#include <stdint.h>
#include <mfd/axp20x.h>
#include <regmap/sunxi-rsb.h>

#define IC_TYPE_REG       0x03
#define IC_TYPE_MASK      0xcf
#define IC_TYPE_VALUE     0x41

#define AXP20X_RSB_RTADDR 0x2d

static int
axp20x_probe(const struct device *dev)
{
	const struct regmap_device *self = to_regmap_device(dev);
	uint8_t reg;
	int err;

	if ((err = regmap_device_probe(dev)))
		return err;
	if ((err = regmap_read(&self->map, IC_TYPE_REG, &reg)))
		goto err_release;
	if ((reg & IC_TYPE_MASK) != IC_TYPE_VALUE) {
		err = ENODEV;
		goto err_release;
	}

	return SUCCESS;

err_release:
	regmap_device_release(dev);

	return err;
}

static const struct driver axp20x_driver = {
	.probe   = axp20x_probe,
	.release = regmap_device_release,
};

const struct regmap_device axp20x = {
	.dev = {
		.name  = "axp20x",
		.drv   = &axp20x_driver,
		.state = DEVICE_STATE_INIT,
	},
	.map = {
		.dev = &r_rsb.dev,
		.id  = AXP20X_RSB_RTADDR,
	},
};