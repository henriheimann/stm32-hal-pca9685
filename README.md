# PCA9685
Driver for the PCA9685 16 channel 12-bit PWM driver used in this Adafruit product:
https://www.adafruit.com/product/815

## Usage
After connecting the PCA9685 driver to your microcontroller via I2C and initialising the bus using Cube you can use this
library to interact with the driver as shown in the following example:
```c
// Create the handle for the driver.
pca9685_handle_t handle = {
    .i2c_handle = &hi2c1,
    .device_address = PCA9865_I2C_DEFAULT_DEVICE_ADDRESS,
    .inverted = false
};

// Initialise driver (performs basic setup).
pca9685_init(&handle);

// Set PWM frequency.
// The frequency must be between 24Hz and 1526Hz.
pca9685_set_pwm_frequency(&handle, 100.0f);

// Set the channel on and off times.
// The channel must be >= 0 and < 16.
// The on and off times must be >= 0 and < 4096.
// In this example, the duty cycle is (off time - on time) / 4096 = (2048 - 0) / 4096 = 50%
pca9685_set_channel_pwm_times(&handle, 0, 0, 2048);

// If you do not want to set the times by hand you can directly set the duty cycle. The last parameter
// lets you use a logarithmic scale for LED dimming applications.
pca9685_set_channel_duty_cycle(&handle, 1, 0.5f, false); 
```

## Supported Platforms
STM32L0 and STM32L4 are supported. The HAL header includes for other platforms may be added in `pca9685.h`.