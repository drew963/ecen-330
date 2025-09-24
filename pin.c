#include <stdio.h>
#include "soc/reg_base.h" // DR_REG_GPIO_BASE, DR_REG_IO_MUX_BASE
#include "driver/rtc_io.h" // rtc_gpio_*
#include "pin.h"

// TODO: GPIO Matrix Registers - GPIO_OUT_REG, GPIO_OUT_W1TS_REG, ...
// NOTE: Remember to enclose the macro values in parenthesis, as below
#define GPIO_OUT_REG           (DR_REG_GPIO_BASE + 0x04)
#define GPIO_OUT_W1TS_REG      (DR_REG_GPIO_BASE + 0x08)
#define GPIO_OUT_W1TC_REG      (DR_REG_GPIO_BASE + 0x0C)
#define GPIO_OUT1_REG          (DR_REG_GPIO_BASE + 0x10)
#define GPIO_OUT1_W1TS_REG     (DR_REG_GPIO_BASE + 0x14)
#define GPIO_OUT1_W1TC_REG     (DR_REG_GPIO_BASE + 0x18)
#define GPIO_ENABLE_REG        (DR_REG_GPIO_BASE + 0x20)
#define GPIO_ENABLE_W1TS_REG   (DR_REG_GPIO_BASE + 0x24)
#define GPIO_ENABLE_W1TC_REG   (DR_REG_GPIO_BASE + 0x28)
#define GPIO_ENABLE1_REG       (DR_REG_GPIO_BASE + 0x2C)
#define GPIO_ENABLE1_W1TS_REG  (DR_REG_GPIO_BASE + 0x30)
#define GPIO_ENABLE1_W1TC_REG  (DR_REG_GPIO_BASE + 0x34)
#define GPIO_IN_REG            (DR_REG_GPIO_BASE + 0x3C)
#define GPIO_IN1_REG           (DR_REG_GPIO_BASE + 0x40)

#define GPIO_PIN_REG(n)        (DR_REG_GPIO_BASE + 0x88 + ((n) * 4))
#define GPIO_FUNC_OUT_SEL_CFG(n) (DR_REG_GPIO_BASE + 0x530 + (n) * 4)


// TODO: IO MUX Registers
// HINT: Add DR_REG_IO_MUX_BASE with PIN_MUX_REG_OFFSET[n]
#define IO_MUX_REG(n)				(DR_REG_IO_MUX_BASE + PIN_MUX_REG_OFFSET[(n)]) // TODO: Finish this macro

// TODO: IO MUX Register Fields - FUN_WPD, FUN_WPU, ...
#define FUN_WPD_S  7
#define FUN_WPD      (1 << FUN_WPD_S)
#define MCU_SEL_S 12
#define FUN_DRV_S 10
#define FUN_WPU_S 8
#define FUN_IE_S 9
#define FUN_IE       (1 << FUN_IE_S)
#define MCU_SEL(v)   (((v) & 0x3) << MCU_SEL_S)   
#define FUN_DRV(v)   (((v) & 0x3) << FUN_DRV_S)  
#define FUN_WPU      (1 << FUN_WPU_S)   
#define PAD_DRIVER 2

#define REG(r) (*(volatile uint32_t *)(r))
#define REG_BITS 32
// TODO: Finish these macros. HINT: Use the REG() macro.
#define REG_SET_BIT(r,b) (REG(r) |= (uint32_t)(b))
#define REG_CLR_BIT(r,b) (REG(r) &= ~(uint32_t)(b))
#define REG_GET_BIT(r,b) (REG(r) & (uint32_t)(b))


//extra nubmers
#define PIN_RESET_FUN_OUT 0x100
#define FIRST_REG 1
#define SECOND_REG 2
#define RESET_MUX_REG 0x3

// Gives byte offset of IO_MUX Configuration Register
// from base address DR_REG_IO_MUX_BASE
static const uint8_t PIN_MUX_REG_OFFSET[] = {
    0x44, 0x88, 0x40, 0x84, 0x48, 0x6c, 0x60, 0x64, // pin  0- 7
    0x68, 0x54, 0x58, 0x5c, 0x34, 0x38, 0x30, 0x3c, // pin  8-15
    0x4c, 0x50, 0x70, 0x74, 0x78, 0x7c, 0x80, 0x8c, // pin 16-23
    0x90, 0x24, 0x28, 0x2c, 0xFF, 0xFF, 0xFF, 0xFF, // pin 24-31
    0x1c, 0x20, 0x14, 0x18, 0x04, 0x08, 0x0c, 0x10, // pin 32-39
};


// Reset the configuration of a pin to not be an input or an output.
// Pull-up is enabled so the pin does not float.
// Return zero if successful, or non-zero otherwise.
int32_t pin_reset(pin_num_t pin)
{
	if (rtc_gpio_is_valid_gpio(pin)) { // hand-off work to RTC subsystem
			rtc_gpio_deinit(pin);
		rtc_gpio_pullup_en(pin);
		rtc_gpio_pulldown_dis(pin);
	}    // 1. Reset GPIO_PINn_REG: all fields to zero
    REG(GPIO_PIN_REG(pin)) = 0;

    // 2. Reset GPIO_FUNCn_OUT_SEL_CFG_REG: set GPIO_FUNCn_OUT_SEL = 0x100
    REG(GPIO_FUNC_OUT_SEL_CFG(pin)) = PIN_RESET_FUN_OUT;

	REG_CLR_BIT(IO_MUX_REG(pin), FUN_WPD);
	REG_SET_BIT(IO_MUX_REG(pin), MCU_SEL(2) | FUN_DRV(2) | FUN_WPU | (FUN_IE));
		// Now that the pin is reset, set the output level to zero
	return pin_set_level(pin, 0);
}

// Enable or disable a pull-up on the pin.
// Return zero if successful, or non-zero otherwise.
int32_t pin_pullup(pin_num_t pin, bool enable)
{
	if (rtc_gpio_is_valid_gpio(pin)) { // hand-off work to RTC subsystem
		if (enable) return rtc_gpio_pullup_en(pin);
		else return rtc_gpio_pullup_dis(pin);
	}
	// Set the bit specifiied by pin and chagnes fun_wpu
	if (enable) {
		REG_SET_BIT(IO_MUX_REG(pin), (FUN_WPU));
	}
	else {
		REG_CLR_BIT(IO_MUX_REG(pin), (FUN_WPU));
	}
	return 0;
}

// Enable or disable a pull-down on the pin.
// Return zero if successful, or non-zero otherwise.
int32_t pin_pulldown(pin_num_t pin, bool enable)
{
	if (rtc_gpio_is_valid_gpio(pin)) { // hand-off work to RTC subsystem
		if (enable) return rtc_gpio_pulldown_en(pin);
		else return rtc_gpio_pulldown_dis(pin);
	}

	// Set the bit specifiied by pin and chagnes fun_wpd

	if (enable) {
		REG_SET_BIT(IO_MUX_REG(pin), (FUN_WPD));
	}
	else {
			REG_CLR_BIT(IO_MUX_REG(pin), (FUN_WPD));
	}
	return 0;
}

// Enable or disable the pin as an input signal.
// Return zero if successful, or non-zero otherwise.
int32_t pin_input(pin_num_t pin, bool enable)
{
	uint32_t reg = IO_MUX_REG(pin);
	// Sets if enable bool is set and dosent if not
	if (enable) 
	{
		REG_SET_BIT(reg, FUN_IE);
	}
	else 
	{
		REG_CLR_BIT(reg, FUN_IE);
	}


	return 0;
}

// Enable or disable the pin as an output signal.
// Return zero if successful, or non-zero otherwise.
int32_t pin_output(pin_num_t pin, bool enable)
{
	if (enable)
	{
		if (pin < REG_BITS) //check to see if pin is less then 32 and sets the right eneble regester
		{
			REG_SET_BIT(GPIO_ENABLE_REG, 1 << pin);
		}
		else
		{
			REG_SET_BIT(GPIO_ENABLE1_REG, 1 << (pin - REG_BITS));
		}
	}
	else
	{
		if(pin < REG_BITS) // Does the same as above but clears the right regester 
		{
			REG_CLR_BIT(GPIO_ENABLE_REG, (1 << pin));
		}
		else
		{
			REG_CLR_BIT(GPIO_ENABLE1_REG, (1 << (pin - REG_BITS)));
		}
	}
	
	
	return 0;
}

// Enable or disable the pin as an open-drain signal.
// Return zero if successful, or non-zero otherwise.
int32_t pin_odrain(pin_num_t pin, bool enable)
{
	if (enable) {  //makes sure that the pin in moved by the right ammount 
        REG_SET_BIT(GPIO_PIN_REG(pin), (1 << PAD_DRIVER));
    } else {
        REG_CLR_BIT(GPIO_PIN_REG(pin), (1 << PAD_DRIVER));
    }
    return 0;
}

// Sets the output signal level if the pin is configured as an output.
// Return zero if successful, or non-zero otherwise.
int32_t pin_set_level(pin_num_t pin, int32_t level)
{
	 if (pin < REG_BITS ) {
        if (level) {
            REG(GPIO_OUT_W1TS_REG) = (1u << pin);  // set high
        } else {
            REG(GPIO_OUT_W1TC_REG) = (1u << pin);  // set low
        }
    } else {
        // pins 32–39 use the second output register bank
        uint32_t bit = 1u << (pin - REG_BITS);
        if (level) {
            REG(GPIO_OUT1_W1TS_REG) = bit;
        } else {
            REG(GPIO_OUT1_W1TC_REG) = bit;
        }
    }
    return 0;
}

// Gets the input signal level if the pin is configured as an input.
// Return zero or one if successful, or negative otherwise.
int32_t pin_get_level(pin_num_t pin)
{
	if (pin < REG_BITS) {
		return (REG(GPIO_IN_REG) >> pin) &0x1;
	}
	else {
		return (REG(GPIO_IN1_REG) >> (pin - REG_BITS)) & 0x1;
	}
}

// Get the value of the input registers, one pin per bit.
// The two 32-bit input registers are concatenated into a uint64_t.
uint64_t pin_get_in_reg(void)
{
	uint32_t low = REG(GPIO_IN_REG);
	uint32_t high = REG(GPIO_IN1_REG);

    return ((uint64_t)high << REG_BITS) | (uint64_t)low;
}

// Get the value of the output registers, one pin per bit.
// The two 32-bit output registers are concatenated into a uint64_t.
uint64_t pin_get_out_reg(void)
{
  	uint32_t low  = REG(GPIO_OUT_REG);   // GPIO 0–31
    uint32_t high = REG(GPIO_OUT1_REG);  // GPIO 32–39 (lower 8 bits valid)

    // Concatenate: upper bank shifted left by 32 bits
    return ((uint64_t)high << REG_BITS) | (uint64_t)low;

	// TODO: Read the OUT and OUT1 registers, return the concatenated values
}
