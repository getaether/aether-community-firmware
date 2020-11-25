#include "./ble.h"

static void put_ad(uint8_t ad_type, uint8_t ad_len, const void *ad, uint8_t *buf,
       uint8_t *len)
{
    buf[(*len)++] = ad_len + 1;
    buf[(*len)++] = ad_type;

    memcpy(&buf[*len], ad, ad_len);

    *len += ad_len;
}

static void update_ad(void)
{
    uint8_t ad[BLE_HS_ADV_MAX_SZ];
    uint8_t ad_len = 0;
    uint8_t ad_flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    put_ad(BLE_HS_ADV_TYPE_FLAGS, 1, &ad_flags, ad, &ad_len);
    put_ad(BLE_HS_ADV_TYPE_COMP_NAME, sizeof(DEVICE_NAME), DEVICE_NAME, ad, &ad_len);

    ble_gap_adv_set_data(ad, ad_len);
}

static void start_advertise(ble_gap_event_fn *cb)
{
    struct ble_gap_adv_params advp;
    int err;

    update_ad();

    memset(&advp, 0, sizeof advp);
    advp.conn_mode = BLE_GAP_CONN_MODE_UND;
    advp.disc_mode = BLE_GAP_DISC_MODE_GEN;
    err = ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER,
                           &advp, cb, NULL);
    assert(err == 0);
}