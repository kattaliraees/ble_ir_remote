#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "ble_main.h"
#include "ir_nec.h"

#define ESP_INTR_FLAG_DEFAULT 0

static QueueHandle_t gpio_evt_queue = NULL;
static void init_gpio();
static void gpio_task(void *arg);
static void ir_tx_send_command(uint16_t, uint16_t);

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

uint8_t ir_test_code = 0x80;

void app_main(void)
{
    ble_init();
    init_gpio();
    ir_init();

    while (1)
    {
        vTaskDelay(40);
        gpio_set_level(GPIO_NUM_8, 1);
    }
}

void init_gpio()
{
    // heartbeat LED Setup
    esp_rom_gpio_pad_select_gpio(GPIO_NUM_8);
    gpio_set_direction(GPIO_NUM_8, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_8, 0);

    // Pushbutton setup
    gpio_num_t buttons[8] = {
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

    for (uint8_t i = 0; i < 8; i++)
    {

        esp_rom_gpio_pad_select_gpio(buttons[i]);
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

    // create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(gpio_task, "gpio_task", 2048, NULL, 20, NULL);
}

static void gpio_task(void *arg)
{
    uint32_t io_num;
    for (;;)
    {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
        {
            uint32_t addr = 0x3FC0;
            gpio_num_t triggered_gpio = (gpio_num_t)io_num;
            vTaskDelay(100 / portTICK_PERIOD_MS); // De-bouncing delay
            printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(triggered_gpio));
            if (gpio_get_level(triggered_gpio))
            {
                gpio_set_level(GPIO_NUM_8, 0);

                // Sending IR code for each press event
                switch (triggered_gpio)
                {
                case GPIO_NUM_0:
                    ir_tx_send_command(addr, 0x7788); // StandBy
                    break;
                // case GPIO_NUM_2:
                //     ir_tx_send_command(0x88); // PROG1
                //     break;
                case GPIO_NUM_3:
                    ir_tx_send_command(addr, 0x7E81); // VOL+
                    break;
                case GPIO_NUM_4:
                    ir_tx_send_command(addr, 0x7A85); // MUTE
                    break;
                case GPIO_NUM_5:
                    ir_tx_send_command(addr, 0x7986); // RESET
                    break;
                case GPIO_NUM_6:
                    //ir_tx_send_command(addr, 0x7887); // BASS+
                    ir_test_code = ir_test_code + 1;
                    if (ir_test_code != 0x88 && ir_test_code != 0x81 && ir_test_code != 0x85 && ir_test_code != 0x86)
                    {
                        uint16_t ir_cmd = 0x00;
                        uint8_t MSB = ~ir_test_code;
                        uint8_t LSB = ir_test_code;
                        ir_cmd = MSB << 8;
                        ir_cmd = ir_cmd | LSB;
                        ESP_LOGI("TEST", "Sending IR Test cmd - %x \n", ir_cmd);
                        ir_tx_send_command(addr, ir_cmd); // BASS+
                        ir_tx_send_command(addr, ir_cmd); // BASS+
                        ir_tx_send_command(addr, ir_cmd); // BASS+
                    }
                    
                    
                    break;
                case GPIO_NUM_7:
                    ir_tx_send_command(addr, 0x7689); // BASS-
                    break;
                case GPIO_NUM_10:
                    ir_tx_send_command(addr, 0x7F80); // VOL-
                    break;
                default:
                    break;
                }
            }
            else if (triggered_gpio == GPIO_NUM_2)
            // PROG1 is ACTIVE LOW
            {
                if (!gpio_get_level(triggered_gpio))
                {
                    gpio_set_level(GPIO_NUM_8, 0);
                    gpio_set_level(GPIO_NUM_8, 0);
                    ir_tx_send_command(addr, 0x88); // PROG1
                }
            }
            else
            {
            }
        }
    }
}

static void ir_tx_send_command(uint16_t addr, uint16_t cmd)
{
    ir_send_nec(addr, cmd);
}