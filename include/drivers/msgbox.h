/*
 * Copyright © 2017-2019 The Crust Firmware Authors.
 * SPDX-License-Identifier: BSD-3-Clause OR GPL-2.0-only
 */

#ifndef DRIVERS_MSGBOX_H
#define DRIVERS_MSGBOX_H

#include <device.h>
#include <intrusive.h>
#include <stdbool.h>
#include <stdint.h>

#define MSGBOX_OPS(dev) \
	(&container_of((dev)->drv, const struct msgbox_driver, drv)->ops)

struct msgbox_driver_ops {
	void (*ack_rx)(const struct device *dev, uint8_t chan);
	bool (*last_tx_done)(const struct device *dev, uint8_t chan);
	int  (*receive)(const struct device *dev, uint8_t chan,
	                uint32_t *message);
	int  (*send)(const struct device *dev, uint8_t chan,
	             uint32_t message);
};

struct msgbox_driver {
	struct driver            drv;
	struct msgbox_driver_ops ops;
};

/**
 * Acknowledge a message received on a message box channel.
 *
 * @param dev  The message box device.
 * @param chan The message box channel.
 */
static inline void
msgbox_ack_rx(const struct device *dev, uint8_t chan)
{
	MSGBOX_OPS(dev)->ack_rx(dev, chan);
}

/**
 * Check if the last transmission on a message box channel has completed, or if
 * it is still pending. A message is pending until the reception IRQ has been
 * cleared on the remote interface.
 *
 * @param dev  The message box device.
 * @param chan The message box channel.
 */
static inline bool
msgbox_last_tx_done(const struct device *dev, uint8_t chan)
{
	return MSGBOX_OPS(dev)->last_tx_done(dev, chan);
}

/**
 * Receive a message via a message box channel.
 *
 * @param dev     The message box device.
 * @param chan    The channel to use within the message box.
 * @param message The message to receive.
 */
static inline int
msgbox_receive(const struct device *dev, uint8_t chan, uint32_t *message)
{
	return MSGBOX_OPS(dev)->receive(dev, chan, message);
}

/**
 * Send a message via a message box channel.
 *
 * @param dev     The message box device.
 * @param chan    The channel to use within the message box.
 * @param message The message to send.
 */
static inline int
msgbox_send(const struct device *dev, uint8_t chan, uint32_t message)
{
	return MSGBOX_OPS(dev)->send(dev, chan, message);
}

#endif /* DRIVERS_MSGBOX_H */
