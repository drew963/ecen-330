#include <stdio.h>
#include <stdint.h>
#include "esp_adc/adc_oneshot.h"
#include "joy.h"

#define EXAMPLE_ADC1_CHAN0    ADC_CHANNEL_6
#define EXAMPLE_ADC1_CHAN1    ADC_CHANNEL_7    
#define NUM_SAMPLES 10

// Global variables to store joystick center position
static int_fast32_t joy_center_x = 0;
static int_fast32_t joy_center_y = 0;

// Global ADC handle
static adc_oneshot_unit_handle_t adc1_handle = NULL;

// Helper function to average multiple ADC reads for more stable center calibration
static int_fast32_t average_adc_read(adc_channel_t channel) {
    int_fast32_t sum = 0;
    int_fast32_t raw = 0;
    
    for (int i = 0; i < NUM_SAMPLES; ++i) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, channel, &raw));
        sum += raw;
    }
    
    return sum / NUM_SAMPLES;
}

// Initialize the joystick driver. Must be called before use.
// May be called multiple times. Return if already initialized.
// Return zero if successful, or non-zero otherwise.
int32_t joy_init(void)
{
    // Check if already initialized
    if (adc1_handle != NULL) {
        return 0;
    }
    
    // Configure ADC one-shot unit (ADC Unit 1, ULP mode disabled)
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));
    
    // Configure ADC channels (default bitwidth, 12 dB attenuation)
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, EXAMPLE_ADC1_CHAN0, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, EXAMPLE_ADC1_CHAN1, &config));
    
    // Calculate and save center position by averaging multiple reads
    joy_center_x = average_adc_read(EXAMPLE_ADC1_CHAN0);
    joy_center_y = average_adc_read(EXAMPLE_ADC1_CHAN1);
    
    return 0;
}

// Free resources used by the joystick (ADC unit).
// Return zero if successful, or non-zero otherwise.
int32_t joy_deinit(void)
{
    // Delete ADC Unit 1 if handle is not NULL
    if (adc1_handle != NULL) {
        ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
        adc1_handle = NULL;
    }
    
    return 0;
}

// Get the current joystick displacement relative to center position.
// xd and yd are output parameters that will be set to the displacement values.
void joy_get_displacement(int32_t *xd, int32_t *yd) 
{
    int_fast32_t raw_x = 0;
    int_fast32_t raw_y = 0;
    
    // Read current position from ADC channels
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, EXAMPLE_ADC1_CHAN0, &raw_x));
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, EXAMPLE_ADC1_CHAN1, &raw_y));
    
    // Calculate displacement relative to center position
    *xd = raw_x - joy_center_x;
    *yd = raw_y - joy_center_y;
}
