#include "pca9685.h"

#include <math.h>
#include <assert.h>
#include <string.h>

#define PCA9685_SET_BIT_MASK(BYTE, MASK)      ((BYTE) |= (uint8_t)(MASK))
#define PCA9685_CLEAR_BIT_MASK(BYTE, MASK)    ((BYTE) &= (uint8_t)(~(uint8_t)(MASK)))
#define PCA9685_READ_BIT_MASK(BYTE, MASK)     ((BYTE) & (uint8_t)(MASK))

/**
 * Registers addresses.
 */
typedef enum
{
	PCA9685_REGISTER_MODE1 = 0x00,
	PCA9685_REGISTER_MODE2 = 0x01,
	PCA9685_REGISTER_LED0_ON_L = 0x06,
	PCA9685_REGISTER_ALL_LED_ON_L = 0xfa,
	PCA9685_REGISTER_ALL_LED_ON_H = 0xfb,
	PCA9685_REGISTER_ALL_LED_OFF_L = 0xfc,
	PCA9685_REGISTER_ALL_LED_OFF_H = 0xfd,
	PCA9685_REGISTER_PRESCALER = 0xfe
} pca9685_register_t;

/**
 * Bit masks for the mode 1 register.
 */
typedef enum
{
	PCA9685_REGISTER_MODE1_SLEEP = (1u << 4u),
	PCA9685_REGISTER_MODE1_RESTART = (1u << 7u)
} pca9685_register_mode1_t;

/**
 * Logarithmic dimming table, mapping from 255 inputs to 12 bit PWM values.
 */
static const uint16_t CIEL_8_12[] = {
		0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 18, 20, 21, 23, 25, 27, 28, 30, 32, 34, 36, 37, 39, 41, 43, 45, 47, 49, 52,
		54, 56, 59, 61, 64, 66, 69, 72, 75, 77, 80, 83, 87, 90, 93, 97, 100, 103, 107, 111, 115, 118, 122, 126, 131,
		135, 139, 144, 148, 153, 157, 162, 167, 172, 177, 182, 187, 193, 198, 204, 209, 215, 221, 227, 233, 239, 246,
		252, 259, 265, 272, 279, 286, 293, 300, 308, 315, 323, 330, 338, 346, 354, 362, 371, 379, 388, 396, 405, 414,
		423, 432, 442, 451, 461, 471, 480, 490, 501, 511, 521, 532, 543, 554, 565, 576, 587, 599, 610, 622, 634, 646,
		658, 670, 683, 696, 708, 721, 734, 748, 761, 775, 789, 802, 817, 831, 845, 860, 875, 890, 905, 920, 935, 951,
		967, 983, 999, 1015, 1032, 1048, 1065, 1082, 1099, 1117, 1134, 1152, 1170, 1188, 1206, 1225, 1243, 1262, 1281,
		1301, 1320, 1340, 1359, 1379, 1400, 1420, 1441, 1461, 1482, 1504, 1525, 1547, 1568, 1590, 1613, 1635, 1658,
		1681, 1704, 1727, 1750, 1774, 1798, 1822, 1846, 1871, 1896, 1921, 1946, 1971, 1997, 2023, 2049, 2075, 2101,
		2128, 2155, 2182, 2210, 2237, 2265, 2293, 2322, 2350, 2379, 2408, 2437, 2467, 2497, 2527, 2557, 2587, 2618,
		2649, 2680, 2712, 2743, 2775, 2807, 2840, 2872, 2905, 2938, 2972, 3006, 3039, 3074, 3108, 3143, 3178, 3213,
		3248, 3284, 3320, 3356, 3393, 3430, 3467, 3504, 3542, 3579, 3617, 3656, 3694, 3733, 3773, 3812, 3852, 3892,
		3932, 3973, 4013, 4055, 4095
};

static bool pca9685_write_u8(pca9685_handle_t *handle, uint8_t address, uint8_t value)
{
	uint8_t data[] = {address, value};
	return HAL_I2C_Master_Transmit(handle->i2c_handle, handle->device_address, data, 2, PCA9685_I2C_TIMEOUT) == HAL_OK;
}

static bool pca9685_write_data(pca9685_handle_t *handle, uint8_t address, uint8_t *data, size_t length)
{
    if (length == 0 || length > 4) {
        return false;
    }

    uint8_t transfer[5];
    transfer[0] = address;

    memcpy(&transfer[1], data, length);

    return HAL_I2C_Master_Transmit(handle->i2c_handle, handle->device_address, transfer, length + 1, PCA9685_I2C_TIMEOUT) == HAL_OK;
}

static bool pca9685_read_u8(pca9685_handle_t *handle, uint8_t address, uint8_t *dest)
{
	if (HAL_I2C_Master_Transmit(handle->i2c_handle, handle->device_address, &address, 1, PCA9685_I2C_TIMEOUT) != HAL_OK) {
		return false;
	}

	return HAL_I2C_Master_Receive(handle->i2c_handle, handle->device_address, dest, 1, PCA9685_I2C_TIMEOUT) == HAL_OK;
}

bool pca9685_init(pca9685_handle_t *handle)
{
	assert(handle->i2c_handle != NULL);

	bool success = true;

	// Set mode registers to default values (Auto-Increment, Sleep, Open-Drain).
	uint8_t mode1_reg_default_value = 0b00110000u;
	uint8_t mode2_reg_default_value = 0b00000000u;

	if (handle->inverted) {
		mode2_reg_default_value |= 0b00010000u;
	}

	success &= pca9685_write_u8(handle, PCA9685_REGISTER_MODE1, mode1_reg_default_value);
	success &= pca9685_write_u8(handle, PCA9685_REGISTER_MODE2, mode2_reg_default_value);

    // Turn all channels off to begin with.
    uint8_t data[4] = { 0x00, 0x00, 0x00, 0x10 };
    success &= pca9685_write_data(handle, PCA9685_REGISTER_ALL_LED_ON_L, data, 4);

	success &= pca9685_set_pwm_frequency(handle, 1000);
	success &= pca9685_wakeup(handle);

	return success;
}

bool pca9685_is_sleeping(pca9685_handle_t *handle, bool *sleeping)
{
	bool success = true;

	// Read the current state of the mode 1 register.
	uint8_t mode1_reg;
	success &= pca9685_read_u8(handle, PCA9685_REGISTER_MODE1, &mode1_reg);

	// Check if the sleeping bit is set.
	*sleeping = PCA9685_READ_BIT_MASK(mode1_reg, PCA9685_REGISTER_MODE1_SLEEP);

	return success;
}

bool pca9685_sleep(pca9685_handle_t *handle)
{
	bool success = true;

	// Read the current state of the mode 1 register.
	uint8_t mode1_reg;
	success &= pca9685_read_u8(handle, PCA9685_REGISTER_MODE1, &mode1_reg);

	// Don't write the restart bit back and set the sleep bit.
	PCA9685_CLEAR_BIT_MASK(mode1_reg, PCA9685_REGISTER_MODE1_RESTART);
	PCA9685_SET_BIT_MASK(mode1_reg, PCA9685_REGISTER_MODE1_SLEEP);
	success &= pca9685_write_u8(handle, PCA9685_REGISTER_MODE1, mode1_reg);

	return success;
}

bool pca9685_wakeup(pca9685_handle_t *handle)
{
	bool success = true;

	// Read the current state of the mode 1 register.
	uint8_t mode1_reg;
	success &= pca9685_read_u8(handle, PCA9685_REGISTER_MODE1, &mode1_reg);

	bool restart_required = PCA9685_READ_BIT_MASK(mode1_reg, PCA9685_REGISTER_MODE1_RESTART);

	// Clear the restart bit for now and clear the sleep bit.
	PCA9685_CLEAR_BIT_MASK(mode1_reg, PCA9685_REGISTER_MODE1_RESTART);
	PCA9685_CLEAR_BIT_MASK(mode1_reg, PCA9685_REGISTER_MODE1_SLEEP);
	success &= pca9685_write_u8(handle, PCA9685_REGISTER_MODE1, mode1_reg);

	if (restart_required) {

		// Oscillator requires at least 500us to stabilise, so wait 1ms.
		HAL_Delay(1);

		PCA9685_SET_BIT_MASK(mode1_reg, PCA9685_REGISTER_MODE1_RESTART);
		success &= pca9685_write_u8(handle, PCA9685_REGISTER_MODE1, mode1_reg);
	}

	return success;
}

bool pca9685_set_pwm_frequency(pca9685_handle_t *handle, float frequency)
{
	assert(frequency >= 24);
	assert(frequency <= 1526);

	bool success = true;

	// Calculate the prescaler value (see datasheet page 25)
	uint8_t prescaler = (uint8_t)roundf(25000000.0f / (4096 * frequency)) - 1;

	bool already_sleeping;
	success &= pca9685_is_sleeping(handle, &already_sleeping);

	// The prescaler can only be changed in sleep mode.
	if (!already_sleeping) {
		success &= pca9685_sleep(handle);
	}

	// Write the new prescaler value.
	success &= pca9685_write_u8(handle, PCA9685_REGISTER_PRESCALER, prescaler);

	// If the device wasn't sleeping, return from sleep mode.
	if (!already_sleeping) {
		success &= pca9685_wakeup(handle);
	}

	return success;
}

bool pca9685_set_channel_pwm_times(pca9685_handle_t *handle, unsigned channel, unsigned on_time, unsigned off_time)
{
	assert(channel >= 0);
	assert(channel < 16);

	assert(on_time >= 0);
	assert(on_time <= 4096);

	assert(off_time >= 0);
	assert(off_time <= 4096);

	uint8_t data[4] = { on_time, on_time >> 8u, off_time, off_time >> 8u };
	return pca9685_write_data(handle, PCA9685_REGISTER_LED0_ON_L + channel * 4, data, 4);
}

bool pca9685_set_channel_duty_cycle(pca9685_handle_t *handle, unsigned channel, float duty_cycle, bool logarithmic)
{
	assert(duty_cycle >= 0.0);
	assert(duty_cycle <= 1.0);

	if (duty_cycle == 0.0) {
		return pca9685_set_channel_pwm_times(handle, channel, 0, 4096); // Special value for always off
	} else if (duty_cycle == 1.0) {
		return pca9685_set_channel_pwm_times(handle, channel, 4096, 0); // Special value for always on
	} else {

		unsigned required_on_time;

		if (logarithmic) {
			required_on_time = CIEL_8_12[(unsigned)roundf(255 * duty_cycle)];
		} else {
			required_on_time = (unsigned)roundf(4095 * duty_cycle);
		}

		// Offset on and off times depending on channel to minimise current spikes.
		unsigned on_time = (channel == 0) ? 0 : (channel * 256) - 1;
		unsigned off_time = (on_time + required_on_time) & 0xfffu;

		return pca9685_set_channel_pwm_times(handle, channel, on_time, off_time);
	}
}
