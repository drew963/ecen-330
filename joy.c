#include <stdio.h>
#include <stdint.h>
#include "esp_adc/adc_oneshot.h"
#include "joy.h"

#define EXAMPLE_ADC1_CHAN0    ADC_CHANNEL_6
#define EXAMPLE_ADC1_CHAN1    ADC_CHANNEL_7    

#define NUM_SAMPLES 10

static int_fast32_t joy_center_x = 0;
static int_fast32_t joy_center_y = 0;
adc_oneshot_unit_handle_t adc1_handle = NULL; 

static int32_t average_adc_read(adc_channel_t channel) {
    int32_t sum = 0;
    int32_t raw = 0;

    for (int8_t i = 0; i < NUM_SAMPLES; ++i) {
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


    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
    .bitwidth = ADC_BITWIDTH_DEFAULT,
    .atten = ADC_ATTEN_DB_12,

    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, EXAMPLE_ADC1_CHAN0, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, EXAMPLE_ADC1_CHAN1, &config));


    joy_center_x = average_adc_read(EXAMPLE_ADC1_CHAN0);
    joy_center_y = average_adc_read(EXAMPLE_ADC1_CHAN1);

}

// Free resources used by the joystick (ADC unit).
// Return zero if successful, or non-zero otherwise.
int32_t joy_deinit(void)
{
    if(adc1_handle != NULL) 
    {
        ESP_ERROR_CHECK(adc_oneshot_del_uint(adc1_handle));
        adc1_handle = NULL;
        return 0;
    }
}

void joy_get_displacement(int32_t *xd, int *yd) 
{
    int_fast32_t raw_x = 0;
    int_fast32_t raw_y = 0;
    adc_oneshot_read((adc1_handle, EXAMPLE_ADC1_CHAN0, &raw_x));
    adc_oneshot_read((adc1_handle, EXAMPLE_ADC1_CHAN1, &raw_y));

    *xd = raw_x - joy_center_x;
    *yd = raw_y - joy_center_y;
}
