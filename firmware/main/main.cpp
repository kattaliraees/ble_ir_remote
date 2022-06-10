#include <stdio.h>
#include <string.h>
#include <array>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#define ESP_INTR_FLAG_DEFAULT 0

extern "C"
{
    void app_main(void);
}

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
            printf("GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(GPIO_NUM_0));
        }
    }
}

void app_main(void)
{

    // zero-initialize the config structure.
    gpio_config_t io_conf = {};
    // disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    // bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = (1ULL << 9);
    // disable pull-down mode
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    // disable pull-up mode
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    // configure GPIO with the given settings
    // gpio_config(&io_conf);

    gpio_pad_select_gpio(GPIO_NUM_8);
    gpio_set_direction(GPIO_NUM_8, GPIO_MODE_OUTPUT);

    gpio_set_level(GPIO_NUM_8, 0);

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
        gpio_isr_handler_add(buttons[i], gpio_isr_handler, (void *)0);
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
