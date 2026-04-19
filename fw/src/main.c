#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/printk.h>

/* ---------------- BUTTONS ---------------- */
#define BUTTON_D7_NODE DT_NODELABEL(button_d7)
#define BUTTON_D8_NODE DT_NODELABEL(button_d8)

static const struct gpio_dt_spec button_d7 =
    GPIO_DT_SPEC_GET(BUTTON_D7_NODE, gpios);

static const struct gpio_dt_spec button_d8 =
    GPIO_DT_SPEC_GET(BUTTON_D8_NODE, gpios);

/* ---------------- DS18B20 ---------------- */
#define TEMP_NODE DT_NODELABEL(ds18b20)
static const struct device *temp_dev = DEVICE_DT_GET(TEMP_NODE);

int main(void)
{
    printk("Multi-test: Buttons + DS18B20 + ADC\n");

    /* -------- Buttons -------- */
    gpio_pin_configure_dt(&button_d7, GPIO_INPUT);
    gpio_pin_configure_dt(&button_d8, GPIO_INPUT);

    /* -------- DS18B20 -------- */
    if (!device_is_ready(temp_dev))
    {
        printk("DS18B20 not ready\n");
    }


    while (1)
    {

        /* -------- BUTTON READ -------- */
        int d7 = gpio_pin_get_dt(&button_d7);
        int d8 = gpio_pin_get_dt(&button_d8);

        /* -------- TEMP READ -------- */
        struct sensor_value temp;
        int temp_valid = 0;

        if (sensor_sample_fetch(temp_dev) == 0)
        {
            k_msleep(800); // ensure conversion done

            if (sensor_channel_get(temp_dev,
                                   SENSOR_CHAN_AMBIENT_TEMP,
                                   &temp) == 0)
            {
                temp_valid = 1;
            }
        }
        
        /* -------- PRINT -------- */
        printk("D7:%d D8:%d | ", d7, d8);

        if (temp_valid)
        {
            printk("Temp:%d.%06dC | \n",
                   temp.val1, temp.val2);
        }
        else
        {
            printk("Temp:ERR | \n");
        }

        k_sleep(K_SECONDS(2));
    }
}