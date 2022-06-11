#include <stdio.h>
#include <string.h>
#include <array>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "driver/rmt.h"
#include "ir_tools.h"

#define ESP_INTR_FLAG_DEFAULT 0


static rmt_channel_t example_tx_channel = RMT_CHANNEL_0;
static const char *TAG = "example";



extern "C"
{
    void app_main(void);
}
static void example_ir_tx_task(void *arg);
static QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    gpio_set_level(GPIO_NUM_8, 0);
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task_example(void *arg)
{
    uint32_t io_num;
    for (;;)
    {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
        {
            printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level((gpio_num_t)io_num));
        }
    }
}

void app_main(void)
{
    //heartbeat LED Setup
    gpio_pad_select_gpio(GPIO_NUM_8);
    gpio_set_direction(GPIO_NUM_8, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_8, 0);

    //Pushbutton setup
    std::array<gpio_num_t, 8> buttons = {
        GPIO_NUM_0,
        GPIO_NUM_2,
        GPIO_NUM_3,
        GPIO_NUM_4,
        GPIO_NUM_5,
        GPIO_NUM_6,
        GPIO_NUM_7,
        GPIO_NUM_10};

    // install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

    for (uint8_t i = 0; i < buttons.size(); i++)
    {
        
        gpio_pad_select_gpio(buttons[i]);
        gpio_intr_enable(buttons[i]);
        gpio_set_direction(buttons[i], GPIO_MODE_INPUT);
        if (buttons[i] == GPIO_NUM_2)
        {
            gpio_set_intr_type(buttons[i], GPIO_INTR_NEGEDGE);
        }
        else
        {
            gpio_pullup_dis(buttons[i]);
            gpio_pulldown_en(buttons[i]);
            gpio_set_intr_type(buttons[i], GPIO_INTR_POSEDGE);
        };
        gpio_isr_handler_add(buttons[i], gpio_isr_handler, (void *)buttons[i]);
    }

    // gpio_pad_select_gpio(GPIO_NUM_0);
    // gpio_intr_enable(GPIO_NUM_0);
    // gpio_set_direction(GPIO_NUM_0, GPIO_MODE_INPUT);
    // gpio_set_intr_type(GPIO_NUM_0, GPIO_INTR_POSEDGE);
    // gpio_pullup_dis(GPIO_NUM_0);
    // gpio_pulldown_en(GPIO_NUM_0);

    // create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    // start gpio task
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 20, NULL);
    //xTaskCreate(example_ir_tx_task, "ir_tx_task", 2048, NULL, 10, NULL);


    // gpio_isr_handler_add(GPIO_NUM_0, gpio_isr_handler, (void *)0);

    int i = 0;
    while (1)
    {
        printf("[%d] Hello world!\n", i);
        i++;
        gpio_set_level(GPIO_NUM_8, 1);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        // gpio_set_level(GPIO_NUM_8, 0);
        // vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}
/*

    IrSender.sendNEC(sAddress, 0x88, sRepeats);
    delay(1000);
    IrSender.sendNEC(sAddress, 0x86, sRepeats);
    delay(5000);
    IrSender.sendNEC(sAddress, 0x81, sRepeats);
    delay(1000);
   DsAddress, 0x81, sRepeats);
    d               lllll;;;;;;;;   ;   ;;;;;;;;;;tjkjthetlay(1000);
    IrSender.sendNEC(sAddress, 0x81, sRepeats);fbd 
*/

static void example_ir_tx_task(void *arg)
{
    uint32_t addr = 0x3FC0;
    uint32_t cmd = 0x88;
    rmt_item32_t *items = NULL;
    size_t length = 0;
    ir_builder_t *ir_builder = NULL;

    rmt_config_t rmt_tx_config = RMT_DEFAULT_CONFIG_TX(GPIO_NUM_1, example_tx_channel);
    rmt_tx_config.tx_config.carrier_en = true;
    rmt_config(&rmt_tx_config);
    rmt_driver_install(example_tx_channel, 0, 0);
    ir_builder_config_t ir_builder_config = IR_BUILDER_DEFAULT_CONFIG((ir_dev_t)example_tx_channel);
    ir_builder_config.flags |= IR_TOOLS_FLAGS_PROTO_EXT; // Using extended IR protocols (both NEC and RC5 have extended version)
//#if CONFIG_EXAMPLE_IR_PROTOCOL_NEC
    ir_builder = ir_builder_rmt_new_nec(&ir_builder_config);
// #elif CONFIG_EXAMPLE_IR_PROTOCOL_RC5
//     ir_builder = ir_builder_rmt_new_rc5(&ir_builder_config);
// #endif
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(2000));
        ESP_LOGI(TAG, "Send command 0x%x to address 0x%x", cmd, addr);
        // Send new key code
        ESP_ERROR_CHECK(ir_builder->build_frame(ir_builder, addr, cmd));
        ESP_ERROR_CHECK(ir_builder->get_result(ir_builder, &items, &length));
        //To send data according to the waveform items.
        rmt_write_items(example_tx_channel, items, length, false);
        // Send repeat code
        vTaskDelay(pdMS_TO_TICKS(ir_builder->repeat_period_ms));
        ESP_ERROR_CHECK(ir_builder->build_repeat_frame(ir_builder));
        ESP_ERROR_CHECK(ir_builder->get_result(ir_builder, &items, &length));
        rmt_write_items(example_tx_channel, items, length, false);
        cmd++;
    }
    ir_builder->del(ir_builder);
    rmt_driver_uninstall(example_tx_channel);
    vTaskDelete(NULL);
}