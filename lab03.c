#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gptimer.h"
#include "hw.h"
#include "lcd.h"
#include "pin.h"
#include "watch.h"
#include "esp_timer.h"


#define BUTTON_A 32
#define BUTTON_B 33
#define BUTTON_START 39



static const char *TAG = "lab03";

volatile uint32_t timer_ticks = 0;
volatile bool running = false;

static int64_t io_config_time      = 0;
static int64_t timer_config_time   = 0;
static int64_t log_call_time       = 0;

volatile int64_t isr_max; // Maximum ISR execution time (us)
volatile int32_t isr_cnt; // Count of ISR invocations 


gptimer_handle_t gptimer = NULL;
gptimer_config_t timer_config = {
    .clk_src = GPTIMER_CLK_SRC_DEFAULT, // Select the default clock source
    .direction = GPTIMER_COUNT_UP,      // Counting direction is up
    .resolution_hz = 1 * 1000 * 1000,   // Resolution is 1 MHz, i.e., 1 tick equals 1 microsecond
};



static bool my_timer_alarm_callback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_ctx)
    {

        int64_t start = esp_timer_get_time();       

        if (pin_get_level(BUTTON_A) == 0) {
            running = true;
        }
        else if (pin_get_level(BUTTON_B) == 0) {
            running = false;
        }
        else if (pin_get_level(BUTTON_START) == 0) {
            running = false;
            timer_ticks = 0;
        }

        if (running) {
            timer_ticks++;
        }

         // Measure and record
        int64_t duration = esp_timer_get_time() - start;

        isr_cnt++;                                // Count this invocation
        if (duration > isr_max) {                 // Update max if this one is longer
            isr_max = duration;
        }
        return false;
    }

gptimer_alarm_config_t alarm_config = {
    .reload_count = 0,      // When the alarm event occurs, the timer will automatically reload to 0
    .alarm_count = 10000, // Set the actual alarm period, since the resolution is 1us, 1000000 represents 1s
    .flags.auto_reload_on_alarm = true, // Enable auto-reload function
};


gptimer_event_callbacks_t cbs = {
    .on_alarm = my_timer_alarm_callback, // Call the user callback function when the alarm event occurs
};



// Main application
void app_main(void)
{
	ESP_LOGI(TAG, "Starting");

    int64_t t_start = esp_timer_get_time();

    pin_reset(BUTTON_A);    // resets and looks for button A
    pin_input(BUTTON_A, true);
    
    pin_reset(BUTTON_B);    // resets and looks for button B
    pin_input(BUTTON_B, true);
    
    pin_reset(BUTTON_START);    // resets and looks for button START
    pin_input(BUTTON_START, true);
    

    io_config_time = esp_timer_get_time() - t_start;
    
    t_start = esp_timer_get_time();

    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer, &cbs, NULL));
    ESP_ERROR_CHECK(gptimer_enable(gptimer));
    ESP_ERROR_CHECK(gptimer_start(gptimer));

    timer_config_time = esp_timer_get_time() - t_start;

    t_start = esp_timer_get_time();
    ESP_LOGI(TAG, "Stopwatch update");
    log_call_time = esp_timer_get_time() - t_start;

    ESP_LOGI(TAG, "I/O config time:       %lld us", io_config_time);
    ESP_LOGI(TAG, "Timer config time:     %lld us", timer_config_time);
    ESP_LOGI(TAG, "ESP_LOGI call time:    %lld us", log_call_time);
    
    
    lcd_init(); // Initialize LCD display
    watch_init(); // Initialize stopwatch face
    for (;;) { // forever update loop
        watch_update(timer_ticks);
        
        static uint32_t last_print_ticks = 0;
        if (timer_ticks - last_print_ticks >= 500) { // about once a second
            last_print_ticks = timer_ticks;
            ESP_LOGI(TAG, "ISR calls: %" PRId32 ", max duration: %" PRId64 " us", isr_cnt, isr_max);
            isr_cnt = 0;
            isr_max = 0;
        }

    }
}
