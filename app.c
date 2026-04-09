#include <em_cmu.h>
#include <em_chip.h>
#include <em_emu.h>
#include <hubble/ble.h>
#include <hubble/hubble.h>
#include "sl_bt_api.h"
#include "sl_sleeptimer.h"
#include "sl_main_init.h"
#include "app_assert.h"
#include "app.h"
#include <stdio.h>

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

// Timer settings to rotate the advertisement (set to 1 hour)
#define ADV_ROTATION_PERIOD_MS (60 * 60 * 1000)
static sl_sleeptimer_timer_handle_t adv_rotation_timer;

// Advertisement data will consist of a hard-coded
#define BLE_ADV_LEN 31 // Max length
#define AD_TYPE_COMPLETE_16BIT_UUIDS 0x03
#define AD_TYPE_SERVICE_DATA 0x16
// Advertising segments go [len, type, data] - we construct this for 2 types:
// 1. AD_TYPE_COMPLETE_16BIT_UUIDS
// 2. AD_TYPE_SERVICE_DATA
// For AD_TYPE_COMPLETE_16BIT_UUIDS we know the length, but for AD_TYPE_SERVICE_DATA
// this will be overwritten later as well as the data set
#define HUBBLE_BLE_ADV_HEADER \
  0x03 , AD_TYPE_COMPLETE_16BIT_UUIDS, 0xA6, 0xFC, /* Hubble UUID */ \
  0x01, AD_TYPE_SERVICE_DATA,
#define HUBBLE_BLE_ADV_HEADER_SIZE 6
static uint8_t adv_data[BLE_ADV_LEN] = {
  HUBBLE_BLE_ADV_HEADER
};

// Must match CONFIG_HUBBLE_KEY_SIZE in CMakeLists.txt (32 = 256-bit, 16 = 128-bit)
static uint8_t master_key[32] = {}; // YOUR KEY HERE

// Stopping/starting BLE must be done in the main loop so use a flag to
// indicate work here.
static volatile bool process_adv_rotation_flag = false;

static const uint8_t hubble_payload[] = "Hello world";

void hubble_ble_adv_update(void) {
  sl_status_t sc = sl_bt_advertiser_stop(advertising_set_handle);
  app_assert_status(sc);

	size_t len = sizeof(adv_data) - HUBBLE_BLE_ADV_HEADER_SIZE;

  int status = hubble_ble_advertise_get(
    hubble_payload,
    sizeof(hubble_payload) - 1,
    &adv_data[HUBBLE_BLE_ADV_HEADER_SIZE],
    &len);
  adv_data[4] = len + 1;
  if (status != 0) {
    printf("hubble_ble_advertise_get error %d\n", status);
    // Log an error here
    return;
  }

  sc = sl_bt_legacy_advertiser_set_data(advertising_set_handle,
                                      0x00 /* sl_bt_advertiser_advertising_data_packet */,
                                      sizeof(adv_data),
                                      adv_data);
  app_assert_status(sc);

  sc = sl_bt_legacy_advertiser_start(advertising_set_handle,
                                    sl_bt_legacy_advertiser_non_connectable);
  app_assert_status(sc);
}

void adv_update_timer_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void)handle;
  (void)data;
  process_adv_rotation_flag = true;
  app_proceed();
}

int start_adv_rotation_timer(void)
{
  // Set up the clock to take the clock source you want
  CMU_ClockSelectSet(cmuClock_RTCC, cmuSelect_LFRCO);
  CMU_ClockEnable(cmuClock_RTCC, true);

  sl_status_t sc = sl_sleeptimer_init();
  app_assert_status(sc);

  uint32_t timeout_ticks;
  sc = sl_sleeptimer_ms32_to_tick(ADV_ROTATION_PERIOD_MS, &timeout_ticks);
  app_assert_status(sc);

  sc = sl_sleeptimer_start_periodic_timer(
    &adv_rotation_timer,
    timeout_ticks,
    adv_update_timer_callback,
    (void*)NULL,
    0,
    0);
  app_assert_status(sc);
  return 0;
}

void configure_advertising(void) {
  sl_status_t sc;

  // Create an advertising set.
  sc = sl_bt_advertiser_create_set(&advertising_set_handle);
  app_assert_status(sc);

  // Set advertising interval to 100ms.
  // Update this to what you want
  sc = sl_bt_advertiser_set_timing(
    advertising_set_handle,
    160, // min. adv. interval (milliseconds * 1.6)
    160, // max. adv. interval (milliseconds * 1.6)
    0,   // adv. duration
    0);  // max. num. adv. events
  app_assert_status(sc);

  // Set advertising to use a non-resolvable private address
  bd_addr addr; // unused
  sc = sl_bt_advertiser_set_random_address(
    advertising_set_handle,
    sl_bt_gap_random_nonresolvable_address, // type
    addr, // unused with non-resolvable random address
    NULL);
  app_assert_status(sc);

  // Start advertising
  hubble_ble_adv_update();
}

// Application Init.
void app_init(void)
{
  /////////////////////////////////////////////////////////////////////////////
  // Put your additional application init code here!                         //
  // This is called once during start-up.                                    //
  /////////////////////////////////////////////////////////////////////////////
  // Using device uptime as the counter source — 0 starts the counter at
  // epoch 0. The SDK tracks time from device boot via hubble_uptime_get().
  hubble_init(0, master_key);
}

// Application Process Action.
void app_process_action(void)
{
  if (app_is_process_required()) {
    /////////////////////////////////////////////////////////////////////////////
    // Put your additional application code here!                              //
    // This is will run each time app_proceed() is called.                     //
    // Do not call blocking functions from here!                               //
    /////////////////////////////////////////////////////////////////////////////

    if (process_adv_rotation_flag) {
      process_adv_rotation_flag = false;
      // Update the advertisement to ensure ephemeral ID and encryption key rotation
      // Cannot be done in interrupt context
      hubble_ble_adv_update();
      printf("Advertisement rotated.\n");
    }
  }
}

/**************************************************************************//**
 * Bluetooth stack event handler.
 * This overrides the default weak implementation.
 *
 * @param[in] evt Event coming from the Bluetooth stack.
 *****************************************************************************/
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  switch (SL_BT_MSG_ID(evt->header)) {
    // -------------------------------
    // This event indicates the device has started and the radio is ready.
    // Do not call any stack command before receiving this boot event!
    case sl_bt_evt_system_boot_id:
      configure_advertising();
      start_adv_rotation_timer();
      printf("Boot complete.\n");
      break;

    // -------------------------------
    // This event indicates that a new connection was opened.
    case sl_bt_evt_connection_opened_id:
      break;

    // -------------------------------
    // This event indicates that a connection was closed.
    case sl_bt_evt_connection_closed_id:
      break;

    ///////////////////////////////////////////////////////////////////////////
    // Add additional event handlers here as your application requires!      //
    ///////////////////////////////////////////////////////////////////////////

    // -------------------------------
    // Default event handler.
    default:
      break;
  }
}
