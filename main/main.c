#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <hap.h>
#include <hap_apple_servs.h>
#include <hap_apple_chars.h>
#include <iot_button.h>
#include <app_wifi.h>
#include <app_hap_setup_payload.h>
#include <si7021.h>

static const char *TAG = "HAP_SI7021";

#define SENSOR_TASK_PRIORITY  1
#define SENSOR_TASK_STACKSIZE 4 * 1024
#define SENSOR_TASK_NAME      "hap_sensor"

#define RESET_GPIO           0

// Define the setup code and setup ID
#define SETUP_CODE "111-22-333"
#define SETUP_ID "ES32"
#define I2C_MASTER_SDA 21
#define I2C_MASTER_SCL 22

#ifndef APP_CPU_NUM
#define APP_CPU_NUM PRO_CPU_NUM
#endif

static i2c_dev_t dev;
static float temperature = 0.0;
static float humidity = 0.0;

static void reset_network_handler(void* arg)
{
    hap_reset_network();
}

static void reset_to_factory_handler(void* arg)
{
    hap_reset_to_factory();
}

static void reset_key_init(uint32_t key_gpio_pin)
{
    button_handle_t handle = iot_button_create(key_gpio_pin, BUTTON_ACTIVE_LOW);
    iot_button_add_on_release_cb(handle, 3, reset_network_handler, NULL);
    iot_button_add_on_press_cb(handle, 10, reset_to_factory_handler, NULL);
}

static int sensor_identify(hap_acc_t *ha)
{
    ESP_LOGI(TAG, "Accessory identified");
    return HAP_SUCCESS;
}

static int sensor_read(hap_char_t *hc, hap_status_t *status_code, void *serv_priv, void *read_priv)
{
    if (hap_req_get_ctrl_id(read_priv)) {
        ESP_LOGI(TAG, "Sensor received read from %s", hap_req_get_ctrl_id(read_priv));
    }

    if (strcmp(hap_char_get_type_uuid(hc), HAP_CHAR_UUID_CURRENT_TEMPERATURE) == 0) {
        hap_val_t new_val = {.f = temperature};
        hap_char_update_val(hc, &new_val);
        *status_code = HAP_STATUS_SUCCESS;
    } else if (strcmp(hap_char_get_type_uuid(hc), HAP_CHAR_UUID_CURRENT_RELATIVE_HUMIDITY) == 0) {
        hap_val_t new_val = {.f = humidity};
        hap_char_update_val(hc, &new_val);
        *status_code = HAP_STATUS_SUCCESS;
    }

    return HAP_SUCCESS;
}

static void sensor_task(void *pvParameters)
{
    esp_err_t res;

    while (1) {
        res = si7021_measure_temperature(&dev, &temperature);
        if (res != ESP_OK)
            ESP_LOGE(TAG, "Could not measure temperature: %d (%s)", res, esp_err_to_name(res));
        else
            ESP_LOGI(TAG, "Temperature: %.2f", temperature);

        res = si7021_measure_humidity(&dev, &humidity);
        if (res != ESP_OK)
            ESP_LOGE(TAG, "Could not measure humidity: %d (%s)", res, esp_err_to_name(res));
        else
            ESP_LOGI(TAG, "Humidity: %.2f", humidity);

        vTaskDelay(pdMS_TO_TICKS(10000)); // Read every 10 seconds
    }
}

static void hap_init_task(void *p)
{
    hap_acc_t *accessory;
    hap_serv_t *temperature_service;
    hap_serv_t *humidity_service;

    hap_cfg_t hap_cfg;
    hap_get_config(&hap_cfg);
    hap_cfg.unique_param = UNIQUE_NAME;
    hap_set_config(&hap_cfg);

    hap_init(HAP_TRANSPORT_WIFI);

    hap_acc_cfg_t cfg = {
        .name = "ESP-SI7021",
        .manufacturer = "James Young",
        .model = "ESP-SI7021",
        .serial_num = "001122334455",
        .fw_rev = "1.0.0",
        .hw_rev = NULL,
        .pv = "1.1.0",
        .identify_routine = sensor_identify,
        .cid = HAP_CID_SENSOR,
    };

    accessory = hap_acc_create(&cfg);

    uint8_t product_data[] = {'E','S','P','3','2','H','A','P'};
    hap_acc_add_product_data(accessory, product_data, sizeof(product_data));

    temperature_service = hap_serv_temperature_sensor_create(temperature);
    hap_serv_add_char(temperature_service, hap_char_name_create("Si7021 Temperature"));
    hap_serv_set_read_cb(temperature_service, sensor_read);
    hap_acc_add_serv(accessory, temperature_service);

    humidity_service = hap_serv_humidity_sensor_create(humidity);
    hap_serv_add_char(humidity_service, hap_char_name_create("Si7021 Humidity"));
    hap_serv_set_read_cb(humidity_service, sensor_read);
    hap_acc_add_serv(accessory, humidity_service);

    hap_add_accessory(accessory);

    reset_key_init(RESET_GPIO);

    hap_set_setup_code(SETUP_CODE);
    hap_set_setup_id(SETUP_ID);

    app_wifi_init();
    
    hap_enable_mfi_auth(HAP_MFI_AUTH_NONE);
    hap_start();
    app_wifi_start(portMAX_DELAY);

    vTaskDelete(NULL);
}

void app_main()
{
    esp_log_level_set("*", ESP_LOG_INFO);

    // Initialize I2C
    ESP_ERROR_CHECK(i2cdev_init());

    // Initialize SI7021
    memset(&dev, 0, sizeof(i2c_dev_t));
    ESP_ERROR_CHECK(si7021_init_desc(&dev, 0, I2C_MASTER_SDA, I2C_MASTER_SCL));

    // Create tasks
    xTaskCreatePinnedToCore(sensor_task, SENSOR_TASK_NAME, SENSOR_TASK_STACKSIZE, NULL, SENSOR_TASK_PRIORITY, NULL, APP_CPU_NUM);
    xTaskCreate(hap_init_task, "hap_init", 4096, NULL, 1, NULL);
}