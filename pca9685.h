#pragma once

#if (defined STM32L011xx) || (defined STM32L021xx) || \
	(defined STM32L031xx) || (defined STM32L041xx) || \
	(defined STM32L051xx) || (defined STM32L052xx) || (defined STM32L053xx) || \
	(defined STM32L061xx) || (defined STM32L062xx) || (defined STM32L063xx) || \
	(defined STM32L071xx) || (defined STM32L072xx) || (defined STM32L073xx) || \
	(defined STM32L081xx) || (defined STM32L082xx) || (defined STM32L083xx)
#include "stm32l0xx_hal.h"
#elif defined (STM32L412xx) || defined (STM32L422xx) || \
	defined (STM32L431xx) || (defined STM32L432xx) || defined (STM32L433xx) || defined (STM32L442xx) || defined (STM32L443xx) || \
	defined (STM32L451xx) || defined (STM32L452xx) || defined (STM32L462xx) || \
	defined (STM32L471xx) || defined (STM32L475xx) || defined (STM32L476xx) || defined (STM32L485xx) || defined (STM32L486xx) || \
    defined (STM32L496xx) || defined (STM32L4A6xx) || \
    defined (STM32L4R5xx) || defined (STM32L4R7xx) || defined (STM32L4R9xx) || defined (STM32L4S5xx) || defined (STM32L4S7xx) || defined (STM32L4S9xx)
#include "stm32l4xx_hal.h"
#else
#error Platform not implemented
#endif

#include <stdbool.h>

#ifndef PCA9685_I2C_TIMEOUT
#define PCA9685_I2C_TIMEOUT 1
#endif

#define PCA9865_I2C_DEFAULT_DEVICE_ADDRESS 0x80

/**
 * Structure defining a handle describing a PCA9685 device.
 */
typedef struct {

	/**
	 * The handle to the I2C bus for the device.
	 */
	I2C_HandleTypeDef *i2c_handle;

	/**
	 * The I2C device address.
	 * @see{PCA9865_I2C_DEFAULT_DEVICE_ADDRESS}
	 */
	uint16_t device_address;

	/**
	 * Set to true to drive inverted.
	 */
	bool inverted;

} pca9685_handle_t;

/**
 * Initialises a PCA9685 device by resetting registers to known values, setting a PWM frequency of 1000Hz, turning
 * all channels off and waking it up.
 * @param handle Handle to a PCA9685 device.
 * @return True on success, false otherwise.
 */
bool pca9685_init(pca9685_handle_t *handle);

/**
 * Tests if a PCA9685 is sleeping.
 * @param handle Handle to a PCA9685 device.
 * @param sleeping Set to the sleeping state of the device.
 * @return True on success, false otherwise.
 */
bool pca9685_is_sleeping(pca9685_handle_t *handle, bool *sleeping);

/**
 * Puts a PCA9685 device into sleep mode.
 * @param handle Handle to a PCA9685 device.
 * @return True on success, false otherwise.
 */
bool pca9685_sleep(pca9685_handle_t *handle);

/**
 * Wakes a PCA9685 device up from sleep mode.
 * @param handle Handle to a PCA9685 device.
 * @return True on success, false otherwise.
 */
bool pca9685_wakeup(pca9685_handle_t *handle);

/**
 * Sets the PWM frequency of a PCA9685 device for all channels.
 * Asserts that the given frequency is between 24 and 1526 Hertz.
 * @param handle Handle to a PCA9685 device.
 * @param frequency PWM frequency to set in Hertz.
 * @return True on success, false otherwise.
 */
bool pca9685_set_pwm_frequency(pca9685_handle_t *handle, float frequency);

/**
 * Sets the PWM on and off times for a channel of a PCA9685 device.
 * Asserts that the given channel is between 0 and 15.
 * Asserts that the on and off times are between 0 and 4096.
 * @param handle Handle to a PCA9685 device.
 * @param channel Channel to set the times for.
 * @param on_time PWM on time of the channel.
 * @param off_time PWM off time of the channel.
 * @return True on success, false otherwise.
 */
bool pca9685_set_channel_pwm_times(pca9685_handle_t *handle, unsigned channel, unsigned on_time, unsigned off_time);

/**
 * Helper function to set the PWM duty cycle for a channel of a PCA9685 device. The duty cycle is either directly
 * converted to a 12-bit value used for the PWM timings, if logarithmic is set to false, or an 8-bit value which is then
 * transformed to a 12-bit value using a look up table for the PWM timings.
 * Asserts that the duty cycle is between 0 and 1.
 * @param handle Handle to a PCA9685 device.
 * @param channel Channel to set the duty cycle of.
 * @param duty_cycle Duty cycle to set.
 * @param logarithmic Set to true to apply logarithmic function.
 * @return True on success, false otherwise.
 */
bool pca9685_set_channel_duty_cycle(pca9685_handle_t *handle, unsigned channel, float duty_cycle, bool logarithmic);
