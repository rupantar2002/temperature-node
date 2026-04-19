#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/printk.h>

#include <string.h>
#include <stdio.h>

#include "u8g2.h"

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

/* ---------------- I2C ---------------- */
#define I2C_NODE DT_NODELABEL(i2c0)
static const struct device *i2c_dev = DEVICE_DT_GET(I2C_NODE);

/* ---------------- U8G2 ---------------- */
static u8g2_t u8g2;

/* ---------------- U8G2 HAL ---------------- */
uint8_t u8x8_byte_zephyr_i2c(u8x8_t *u8x8,
                             uint8_t msg,
                             uint8_t arg_int,
                             void *arg_ptr)
{
    static uint8_t buffer[32];
    static uint8_t idx;
    const struct device *i2c = u8x8_GetUserPtr(u8x8);

    switch (msg)
    {
    case U8X8_MSG_BYTE_INIT:
        idx = 0;
        break;

    case U8X8_MSG_BYTE_START_TRANSFER:
        idx = 0;
        break;

    case U8X8_MSG_BYTE_SEND:
        memcpy(&buffer[idx], arg_ptr, arg_int);
        idx += arg_int;
        break;

    case U8X8_MSG_BYTE_END_TRANSFER:
        i2c_write(i2c, buffer, idx, u8x8->i2c_address >> 1);
        break;

    default:
        return 0;
    }
    return 1;
}

uint8_t u8x8_gpio_and_delay_zephyr(u8x8_t *u8x8,
                                   uint8_t msg,
                                   uint8_t arg_int,
                                   void *arg_ptr)
{
    switch (msg)
    {
    case U8X8_MSG_DELAY_MILLI:
        k_msleep(arg_int);
        break;

    case U8X8_MSG_DELAY_10MICRO:
        k_busy_wait(10);
        break;

    case U8X8_MSG_DELAY_100NANO:
        k_busy_wait(1);
        break;

    default:
        break;
    }
    return 1;
}

/* ---------------- DISPLAY INIT ---------------- */
void display_init(void)
{
    if (!device_is_ready(i2c_dev)) {
        printk("I2C not ready for display\n");
        return;
    }

    u8g2_Setup_ssd1306_i2c_128x64_noname_f(
        &u8g2,
        U8G2_R0,
        u8x8_byte_zephyr_i2c,
        u8x8_gpio_and_delay_zephyr
    );

    u8x8_SetUserPtr(&u8g2.u8x8, (void *)i2c_dev);
    u8x8_SetI2CAddress(&u8g2.u8x8, 0x3C << 1);

    u8g2_InitDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);
}

/* ---------------- DISPLAY UPDATE ---------------- */
void display_update(int d7, int d8, int temp_valid, struct sensor_value *temp)
{
    char line1[32];
    char line2[32];

    if (temp_valid) {
        snprintf(line1, sizeof(line1), "Temp: %d.%02d C",
                 temp->val1, temp->val2 / 10000);
    } else {
        snprintf(line1, sizeof(line1), "Temp: ERR");
    }

    snprintf(line2, sizeof(line2), "D7:%d D8:%d", d7, d8);

    u8g2_ClearBuffer(&u8g2);

    u8g2_SetFont(&u8g2, u8g2_font_ncenB08_tr);
    u8g2_DrawStr(&u8g2, 0, 15, line1);
    u8g2_DrawStr(&u8g2, 0, 35, line2);

    u8g2_SendBuffer(&u8g2);
}

/* ---------------- I2C SCAN ---------------- */
static void i2c_scan(const struct device *dev)
{
    printk("\nI2C Scan Start...\n");

    if (!device_is_ready(dev)) {
        printk("I2C device not ready!\n");
        return;
    }

    int found = 0;

    for (uint8_t addr = 0x03; addr <= 0x77; addr++) {
        if (i2c_write(dev, NULL, 0, addr) == 0) {
            printk("Found device at 0x%02X\n", addr);
            found++;
        }
    }

    printk(found ? "Total devices: %d\n" : "No I2C devices found\n", found);
    printk("I2C Scan Done\n\n");
}

/* ---------------- MAIN ---------------- */
int main(void)
{
    printk("Multi-test: Buttons + DS18B20 + I2C + OLED\n");

    gpio_pin_configure_dt(&button_d7, GPIO_INPUT);
    gpio_pin_configure_dt(&button_d8, GPIO_INPUT);

    if (!device_is_ready(temp_dev)) {
        printk("DS18B20 not ready\n");
    }

    if (!device_is_ready(i2c_dev)) {
        printk("I2C not ready\n");
    } else {
        i2c_scan(i2c_dev);
    }

    display_init();

    while (1)
    {
        int d7 = gpio_pin_get_dt(&button_d7);
        int d8 = gpio_pin_get_dt(&button_d8);

        struct sensor_value temp;
        int temp_valid = 0;

        if (sensor_sample_fetch(temp_dev) == 0)
        {
            k_msleep(800);

            if (sensor_channel_get(temp_dev,
                                   SENSOR_CHAN_AMBIENT_TEMP,
                                   &temp) == 0)
            {
                temp_valid = 1;
            }
        }

        printk("D7:%d D8:%d | ", d7, d8);

        if (temp_valid) {
            printk("Temp:%d.%06dC\n", temp.val1, temp.val2);
        } else {
            printk("Temp:ERR\n");
        }

        /* 🔥 Display update */
        display_update(d7, d8, temp_valid, &temp);

        k_sleep(K_SECONDS(2));
    }
}