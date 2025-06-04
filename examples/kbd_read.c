#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "usb/hid_host.h"
#include "usb/hid_usage_keyboard.h"
#include "usb/usb_host.h"

const char *TAG = "USB_HID";
QueueHandle_t queue;

/**
 * Represents an event
 */
typedef struct {
  struct {
    hid_host_device_handle_t handle;
    hid_host_driver_event_t event;
    void *arg;
  } hid_host_device;
} app_event_queue_t;

/**
 * Represents a key event
 */

typedef struct {
  enum key_state { KEY_STATE_PRESSED = 0x00, KEY_STATE_RELEASED = 0x01 } state;
  uint8_t modifier;
  uint8_t key_code;
} key_event_t;

/**
 * HID Protocol string names
 */
static const char *hid_proto_name_str[] = {"NONE", "KEYBOARD", "MOUSE"};

/**
 * @brief Install the USB host library, handle disconnections
 *
 * @param[in] arg Parent handle to send notification
 */
void usb_lib_task(void *arg) {
  const usb_host_config_t config = {.skip_phy_setup = false,
                                    .intr_flags = ESP_INTR_FLAG_LEVEL1};

  ESP_ERROR_CHECK(usb_host_install(&config));
  xTaskNotifyGive(arg);

  while (true) {
    // Get event
    uint32_t event_flags;
    usb_host_lib_handle_events(portMAX_DELAY, &event_flags);

    ESP_LOGI(TAG, "%lu", event_flags);
    // If no more clients, deregister
    
    if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
      ESP_LOGI(TAG, "Deregistering usb host");
      ESP_ERROR_CHECK(usb_host_device_free_all());
      break;
    } 
    if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE) {
      break;
    }
  }

  // Shut down
  ESP_LOGI(TAG, "USB shutting down...");
  vTaskDelay(10); // Short cleanup delay
  ESP_ERROR_CHECK(usb_host_uninstall());
  vTaskDelete(NULL);
}

/**
 * Puts the HID Device event to the queue
 */
void hid_host_device_callback(hid_host_device_handle_t hid_device_handle,
                              const hid_host_driver_event_t event, void *arg) {
  const app_event_queue_t evt_queue = {.hid_host_device.handle =
                                           hid_device_handle,
                                       .hid_host_device.event = event,
                                       .hid_host_device.arg = arg};

  if (queue) {
    xQueueSend(queue, &evt_queue, 0);
  }
}

void hid_host_keyboard_report_callback(const uint8_t *const data,
                                       const int length) {
  hid_keyboard_input_report_boot_t *kb_report =
      (hid_keyboard_input_report_boot_t *)data;

  if (length < sizeof(hid_keyboard_input_report_boot_t)) {
    return;
  }

  static uint8_t prev_keys[HID_KEYBOARD_KEY_MAX] = {0};
  key_event_t key_event;

  for (int i = 0; i < HID_KEYBOARD_KEY_MAX; i++) {
    if (kb_report->key[i]) {
      ESP_LOGI(TAG, "Key[%d]: %u", i, kb_report->key[i]);
    }
  }
}

void hid_host_interface_callback(hid_host_device_handle_t hid_device_handle,
                                 const hid_host_interface_event_t event,
                                 void *arg) {
  uint8_t data[64] = {0};
  size_t data_length = 0;
  hid_host_dev_params_t dev_params;
  ESP_ERROR_CHECK(hid_host_device_get_params(hid_device_handle, &dev_params));

  switch (event) {
  case HID_HOST_INTERFACE_EVENT_INPUT_REPORT:
    ESP_ERROR_CHECK(hid_host_device_get_raw_input_report_data(
        hid_device_handle, data, 64, &data_length));
    if (dev_params.sub_class == HID_SUBCLASS_BOOT_INTERFACE &&
        dev_params.proto == HID_PROTOCOL_KEYBOARD) {
      hid_host_keyboard_report_callback(data, data_length);
    } else {
      ESP_LOGW(TAG, "Received data for an unsupported interface or protocol");
    }
    break;
  case HID_HOST_INTERFACE_EVENT_DISCONNECTED:
    ESP_LOGI(TAG, "HID Device, protocol '%s' DISCONNECTED",
             hid_proto_name_str[dev_params.proto]);
    ESP_ERROR_CHECK(hid_host_device_close(hid_device_handle));
    break;
  case HID_HOST_INTERFACE_EVENT_TRANSFER_ERROR:
    ESP_LOGI(TAG, "HID Device, protocol '%s' TRANSFER_ERROR",
             hid_proto_name_str[dev_params.proto]);
    break;
  default:
    ESP_LOGE(TAG, "HID Device, protocol '%s' Unhandled event",
             hid_proto_name_str[dev_params.proto]);
    break;
  }
}

/**
 * Handles USB HID Host Device
 */
void hid_host_device_event(hid_host_device_handle_t hid_device_handle,
                           const hid_host_driver_event_t event, void *arg) {
  hid_host_dev_params_t dev_params;
  ESP_ERROR_CHECK(hid_host_device_get_params(hid_device_handle, &dev_params));

  switch (event) {
  case HID_HOST_DRIVER_EVENT_CONNECTED:
    ESP_LOGI(TAG, "HID Device, protocol '%s' CONNECTED",
             hid_proto_name_str[dev_params.proto]);

    const hid_host_device_config_t dev_config = {
        .callback = hid_host_interface_callback,
        .callback_arg = NULL,
    };

    ESP_ERROR_CHECK(hid_host_device_open(hid_device_handle, &dev_config));
    if (HID_SUBCLASS_BOOT_INTERFACE == dev_params.sub_class) {
      ESP_ERROR_CHECK(hid_class_request_set_protocol(hid_device_handle,
                                                     HID_REPORT_PROTOCOL_BOOT));
      if (HID_PROTOCOL_KEYBOARD == dev_params.proto) {
        ESP_ERROR_CHECK(hid_class_request_set_idle(hid_device_handle, 0, 0));
      }
    }
    ESP_ERROR_CHECK(hid_host_device_start(hid_device_handle));
    break;
  default:
    break;
  }
}

void app_main() {
  BaseType_t task_created =
      xTaskCreatePinnedToCore(usb_lib_task, "usblibinstall", 4096,
                              xTaskGetCurrentTaskHandle(), 2, NULL, 0);
  assert(task_created == pdTRUE);

  // Wait for usb_lib_task to complete install
  ulTaskNotifyTake(false, 1000);

  const hid_host_driver_config_t config = {.create_background_task = true,
                                           .task_priority = 5,
                                           .stack_size = 4096,
                                           .core_id = 0,
                                           .callback = hid_host_device_callback,
                                           .callback_arg = NULL};

  ESP_ERROR_CHECK(hid_host_install(&config));

  queue = xQueueCreate(10, sizeof(app_event_queue_t));

  ESP_LOGI(TAG, "WAITING FOR CONNECTION");

  app_event_queue_t evt;

  while (true) {
    if (xQueueReceive(queue, &evt, portMAX_DELAY)) {
      hid_host_device_event(evt.hid_host_device.handle,
                            evt.hid_host_device.event, evt.hid_host_device.arg);
    }
  }
}
