#include <assert.h>

#include "esp_nimble_hci.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "sysinit/sysinit.h"
#include "host/util/util.h"

#define WIFICREDS_CHAR_UUID  0xff01
#define DEVICE_NAME "Aether Lumni"


uint8_t ble_addr_type;
static int adv_callback(struct ble_gap_event *event, void *arg);

char * addr_str(const void *addr);
void print_conn_desc(const struct ble_gap_conn_desc *desc);


void ble_init();