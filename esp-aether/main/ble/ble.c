#include "./ble.h"
#include "../constants.h"
#include "../persistance/persistance.h"
#include "advertise.c"


// 59462f12-9543-9999-12c8-58b459a2712d 
static const ble_uuid128_t AE_WIFI_SERVICE_UUID =
    BLE_UUID128_INIT(0x2d, 0x71, 0xa2, 0x59, 0xb4, 0x58, 0xc8, 0x12,
                     0x99, 0x99, 0x43, 0x95, 0x12, 0x2f, 0x46, 0x59);
// 5c3a659e-897e-45e1-b016-007107c96df7 
static const ble_uuid128_t AE_WIFI_VALUE_CHAR_UUID =
        BLE_UUID128_INIT(0xf7, 0x6d, 0xc9, 0x07, 0x71, 0x00, 0x16, 0xb0,
                         0xe1, 0x45, 0x7e, 0x89, 0x9e, 0x65, 0x3a, 0x5c);

// 5c3a659e-897e-45e1-b016-007107c96df6 
static const ble_uuid128_t AE_WIFI_CONFIGURED_UUID =
        BLE_UUID128_INIT(0xf6, 0x6d, 0xc9, 0x07, 0x71, 0x00, 0x16, 0xb0,
                         0xe1, 0x45, 0x7e, 0x89, 0x9e, 0x65, 0x3a, 0x5c);

// 3EE12EC0-9AF0-43B0-86EB-8F0AB4EE386A
static const ble_uuid128_t AE_METADATA_SERVICE_UUID = 
        BLE_UUID128_INIT(0x6a, 0x38, 0xee, 0xb4, 0x0a, 0x8f, 0xeb, 0x86,
                         0xb0, 0x43, 0xf0, 0x9a, 0xc0, 0x2e, 0xe1, 0x3e);

// 44cbf242-cba1-4ad7-ab1c-2579daaea64a
static const ble_uuid128_t AE_UPDATE_ALLOWED_CHAR_UUID = 
        BLE_UUID128_INIT(0x4a, 0xa6, 0xae, 0xda, 0x79, 0x25, 0x1c, 0xab,
                         0xd7, 0x4a, 0xa1, 0xcb, 0x42, 0xf2, 0xcb, 0x44);

// a73ad85a-8ffb-48f1-8d13-dee93d1adc4b
static const ble_uuid128_t AE_UPDATE_AVAILABLE_CHAR_UUID = 
        BLE_UUID128_INIT(0x4b, 0xdc, 0x1a, 0x3d, 0xe9, 0xde, 0x13, 0x8d,
                         0xf1, 0x48, 0xfb, 0x8f, 0x5a, 0xd8, 0x3a, 0xa7);

// 32cf6c97-36b5-4679-87db-b27b22c9ebfa
static const ble_uuid128_t AE_METADATA_IDENTIFIER_UUID = 
        BLE_UUID128_INIT(0xfa, 0xeb, 0xc9, 0x22, 0x7b, 0xb2, 0xdb, 0x87,
                         0x79, 0x46, 0xb5, 0x36, 0x97, 0x6c, 0xcf, 0x32,);
/* Connection handle */
static uint16_t conn_handle;
const char *BLE_TAG = "AE BLE";

uint16_t write_wifiCreds_point_handle;
uint16_t hrs_hrm_handle;

void ae_wifi_write(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctx, void *arg) {
    char buff[200];
    uint16_t om_len, len;
    int err;

    static uint8_t gatt_svr_sec_test_static_val;

    om_len = OS_MBUF_PKTLEN(ctx->om);

    err = ble_hs_mbuf_to_flat(ctx->om, buff, sizeof(buff), &om_len);

    ESP_LOGI("AE BLE", "Got wifi! Credentials are %s\n", buff);
    set_credentials(buff);

    return err;       
}
void ae_wifi_configured(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
        int err;
        assert(ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR);

        int wifiConfigured = wifi_configured() ? 1 : 0;
        err = os_mbuf_append(ctxt->om, &wifiConfigured, sizeof wifiConfigured);
        return err == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}


void ae_update_identifier(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg) {
    int err;
    assert(ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR);

    err = os_mbuf_append(ctxt->om, &(AE_SERIAL_NUMBER), sizeof AE_SERIAL_NUMBER);
    return err == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}



static const struct ble_gatt_svc_def services[] = {
    {
        /*** Service: Security test. */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &AE_WIFI_SERVICE_UUID.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &AE_WIFI_CONFIGURED_UUID.u,
                .access_cb = ae_wifi_configured,
                .flags = BLE_GATT_CHR_F_READ,
            }, 
            {
                .uuid = &AE_WIFI_VALUE_CHAR_UUID.u,
                .access_cb = ae_wifi_write,
                .flags = BLE_GATT_CHR_F_WRITE,
            }, 
            {
                0, /* No more characteristics in this service. */
            } 
        },
    },

    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &AE_METADATA_SERVICE_UUID.u,
        .characteristics = (struct ble_gatt_chr_def[]) { 
            {
                .uuid = &AE_METADATA_IDENTIFIER_UUID.u,
                .access_cb = ae_update_identifier,
                .flags = BLE_GATT_CHR_F_READ,
            }, 
            {
                0, /* No more characteristics in this service. */
            } 
        },
    },

    {
        0, /* No more services. */
    },
};

static int adv_callback(struct ble_gap_event *event, void *arg) {

	int err;
    struct ble_gap_conn_desc desc;

    switch (event->type) {

    case BLE_GAP_EVENT_CONNECT:
        /* A new connection was established or a connection attempt failed. */
        if (event->connect.status == 0) {
            /* Connection successfully established. */
            ESP_LOGI(BLE_TAG, "Connection established");

            err = ble_gap_conn_find(event->connect.conn_handle, &desc);
            assert(err == 0);
        }
        return 0;	

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(BLE_TAG, "Connection broken! reason: %d", event->disconnect.reason);
       
        conn_handle = BLE_HS_CONN_HANDLE_NONE; /* reset conn_handle */

        /* Connection terminated; resume advertising */
        start_advertise(adv_callback);
        break;

    }

    return 0;
}

static void ble_sync(void) {
    int err;

    err = ble_hs_util_ensure_addr(0);
    assert(err == 0);

    err = ble_hs_id_infer_auto(0, &ble_addr_type);
    assert(err == 0);

    start_advertise(adv_callback);
}

void gatt_callback(struct ble_gatt_register_ctxt *ctx, void *arg) {

}

void ble_host_task( void * param) {
    
    nimble_port_run();

}

int gatt_init() {
    int err;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    err = ble_gatts_count_cfg(services);
    assert(err == 0);

    err = ble_gatts_add_svcs(services);
    assert(err == 0);

    return 0;
}

void ble_init() {

	int err = esp_nimble_hci_and_controller_init();
	if (err != ESP_OK) {
	     ESP_LOGE("BLe", "esp_nimble_hci_and_controller_init() failed with error: %d", err);
	     return;
	}

	nimble_port_init();

    ble_hs_cfg.sync_cb = ble_sync;
    ble_hs_cfg.gatts_register_cb = gatt_callback;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    ble_svc_gap_device_name_set(DEVICE_NAME);


    err = gatt_init();
    assert(err == 0);

	nimble_port_freertos_init(ble_host_task);
}
