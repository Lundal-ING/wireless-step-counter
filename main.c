#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <stdio.h>
/* --- Bluetooth additions --- */
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <bluetooth/services/nus.h>
#include <string.h>

/* BMI160 register addresses */
#define BMI160_REG_CHIP_ID          0x00    /* Chip ID register */
#define BMI160_REG_STEP_CONF_0      0x7A    /* Step detector configuration */
#define BMI160_REG_STEP_CONF_1      0x7B    /* Step detector configuration */
#define BMI160_REG_STEP_CNT_LSB     0x78    /* Internal step count register (lsb) */
#define BMI160_REG_STEP_CNT_MSB     0x79    /* Internal step count register (msb) */
#define BMI160_REG_CMD              0x7E    /* Command register */

/* BMI160 register commands */
#define BMI160_CMD_ACC_NORMAL       0x11    /* Sets accelerometer in normal mode in 0x7E */
#define BMI160_CMD_STEP_CNT_RESET   0xB2    /* Resets step count in 0x7E */
#define BMI160_STEP_CONF_0_NORMAL   0x1D    /* Configures step detector in normal mode */
#define BMI160_STEP_CONF_1_NORMAL   0x0F    /* Configures step detector in normal mode and
                                               enables the step counter through step_cnt_en */

/* I2C device spec from devicetree */
static const struct i2c_dt_spec bmi = I2C_DT_SPEC_GET(DT_NODELABEL(bmi160));

/* Write a single byte to a BMI160 register */
static int bmi160_write_reg(uint8_t reg, uint8_t value)
{
    return i2c_reg_write_byte_dt(&bmi, reg, value);
}

/* Read a single byte from a BMI160 register */
static int bmi160_read_reg(uint8_t reg, uint8_t *value)
{
    return i2c_reg_read_byte_dt(&bmi, reg, value);
}

/* ------------------------------------------------------------------------- */
/* BLE / NUS setup                                                            */
/* ------------------------------------------------------------------------- */

#define DEVICE_NAME     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

/* Holds the active BLE connection, NULL when nobody is connected */
static struct bt_conn *current_conn;

/* Advertising data: flags + device name */
static const struct bt_data ad[] = {
    BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
    BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

/* Scan response data: NUS service UUID so clients can find the service */
static const struct bt_data sd[] = {
    BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_NUS_VAL),
};

/* Called when a BLE central connects to us */
static void on_connected(struct bt_conn *conn, uint8_t err)
{
    if (err) { return; }
    printk("BLE connected!\n");
    current_conn = bt_conn_ref(conn);
}

/* Called when the BLE connection is dropped */
static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
    printk("BLE disconnected\n");
    bt_conn_unref(current_conn);
    current_conn = NULL;
}

/* Register connection callbacks */
BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected    = on_connected,
    .disconnected = on_disconnected,
};

/* Called when the web app sends a message to the board over NUS */
static void on_nus_received(struct bt_conn *conn,
                            const uint8_t *data, uint16_t len)
{
    /* If the app sends "RESET", reset the hardware step counter */
    if (len >= 5 && strncmp((const char *)data, "RESET", 5) == 0) {
        bmi160_write_reg(BMI160_REG_CMD, BMI160_CMD_STEP_CNT_RESET);
        printk("Step counter reset by app!\n");
    }
}

/* NUS callback table */
static struct bt_nus_cb nus_cb = {
    .received = on_nus_received,
};

/* ------------------------------------------------------------------------- */
/* BLE helpers                                                                */
/* ------------------------------------------------------------------------- */

/* Send the current step count to the connected BLE central */
static void ble_send_steps(uint16_t steps)
{
    if (!current_conn) { return; }
    char msg[32];
    int  len = snprintf(msg, sizeof(msg), "STEPS:%u\n", steps);
    bt_nus_send(current_conn, (uint8_t *)msg, len);
}

/* ------------------------------------------------------------------------- */
/* Main                                                                       */
/* ------------------------------------------------------------------------- */

int main(void)
{
    uint8_t  steps_lsb, steps_msb;
    uint16_t steps;
    uint16_t prev_steps = 0;

    /* --- I2C / BMI160 init (from assignment 3) --- */
    if (!device_is_ready(bmi.bus)) {
        printf("I2C device not ready!\n");
        return 0;
    } else {
        printf("I2C device is ready!\n");
    }

    /* Read chip ID to verify I2C communication is working.
       BMI160 should return 0xD1 */
    uint8_t chip_id = 0;
    bmi160_read_reg(BMI160_REG_CHIP_ID, &chip_id);
    printk("BMI160 chip ID: 0x%02X (expected 0xD1)\n", chip_id);
    if (chip_id != 0xD1) {
        printk("WARNING: Unexpected chip ID! I2C may not be reaching the sensor.\n");
    }

    printf("Initializing step counter\n");

    int ret;

    /* Wake up accelerometer - needs 100ms to fully transition to normal mode */
    ret = bmi160_write_reg(BMI160_REG_CMD, BMI160_CMD_ACC_NORMAL);
    printk("ACC_NORMAL write: %d %s\n", ret, ret == 0 ? "OK" : "FAIL");
    k_msleep(100);

    /* Configure step detector in normal mode */
    ret = bmi160_write_reg(BMI160_REG_STEP_CONF_0, BMI160_STEP_CONF_0_NORMAL);
    printk("STEP_CONF_0 write: %d %s\n", ret, ret == 0 ? "OK" : "FAIL");
    k_msleep(20);

    /* Enable step counter (step_cnt_en bit is set in STEP_CONF_1) */
    ret = bmi160_write_reg(BMI160_REG_STEP_CONF_1, BMI160_STEP_CONF_1_NORMAL);
    printk("STEP_CONF_1 write: %d %s\n", ret, ret == 0 ? "OK" : "FAIL");
    k_msleep(20);

    /* Reset step count to 0 at startup - needs 100ms to complete */
    ret = bmi160_write_reg(BMI160_REG_CMD, BMI160_CMD_STEP_CNT_RESET);
    printk("STEP_CNT_RESET write: %d %s\n", ret, ret == 0 ? "OK" : "FAIL");
    k_msleep(100);

    printf("Step counter initialized!\n");

    /* --- BLE init --- */
    int err = bt_enable(NULL);
    if (err) {
        printk("Bluetooth init failed (err %d)\n", err);
        return 0;
    }
    printk("Bluetooth initialized\n");

    /* Register NUS callbacks */
    err = bt_nus_init(&nus_cb);
    if (err) {
        printk("NUS init failed (err %d)\n", err);
        return 0;
    }

    
    err = bt_le_adv_start(BT_LE_ADV_CONN_ONE_TIME, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (err) {
        printk("Advertising start failed (err %d)\n", err);
        return 0;
    }
    printk("Advertising started\n");

    /* --- Main loop --- */
    while (1) {
        /* Read 16-bit step count from BMI160 (assignment 3 logic) */
        bmi160_read_reg(BMI160_REG_STEP_CNT_LSB, &steps_lsb);
        bmi160_read_reg(BMI160_REG_STEP_CNT_MSB, &steps_msb);
        steps = ((uint16_t)steps_msb << 8) | steps_lsb;

        /* Only print / transmit when the count actually changes */
        if (steps != prev_steps) {
            printf("Steps: %d\n", steps);
            ble_send_steps(steps);   /* Send updated count over BLE NUS */
            prev_steps = steps;
        }

        k_msleep(100);
    }

    return 0;
}