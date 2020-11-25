#include "./persistance.h"

#define kCREDENTIALS_LENGHT  "lenght"
#define kCREDENTIALS_DATA	 "data"
#define kNEXT_PARTITION      "partition"
#define kUPDATE_AVAILABLE    "update"
#define kDOWNLOADED_OTA      "downloaded"

#define PRIMARY_PARTITION_ID 0
#define SECONDARY_PARTITION_ID 1

void write_credentials(char* data) {
	nvs_handle_t handle;
	esp_err_t err;
	int32_t lenght = ((int32_t) strlen(data)) + 1;

    err = nvs_open("storage", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return;
    } 

    err = nvs_set_i32(handle, kCREDENTIALS_LENGHT, lenght);
	if(err != ESP_OK) {
		nvs_close(handle);
		return;
	}

	err = nvs_set_str(handle, kCREDENTIALS_DATA, data);
	if(err != ESP_OK) {
		nvs_close(handle);
		return;
	}

	err = nvs_commit(handle);
	if(err != ESP_OK) {
		nvs_close(handle);
		return;
	}

	nvs_close(handle);
}

bool read_credentials(char** data) {
	nvs_handle_t handle;
	esp_err_t err;
	int32_t lenght = 0;
	size_t sLen = 0;

	err = nvs_open("storage", NVS_READWRITE, &handle);
	if (err !=  ESP_OK) {
		nvs_close(handle);
		return false;
	}

	err = nvs_get_i32(handle, kCREDENTIALS_LENGHT, &lenght);
	if (err !=  ESP_OK) {
		nvs_close(handle);
		return false;
	}

	sLen = (size_t) lenght;
	*data = malloc(sLen);

	err = nvs_get_str(handle, kCREDENTIALS_DATA, *data, &sLen);
	if (err !=  ESP_OK) {
		nvs_close(handle);
		return false;
	}

	nvs_close(handle);
	return true;
}

void set_credentials(char* to) {
	write_credentials(to);
	esp_restart();
}

bool get_credentials(char** username, char** password) {
	char* data = "";
	int usernameLengh = 0;
	int passwordLenght = 0;
	int dataLenght = 0;
	int separatorIndex = 0;

	if(!read_credentials(&data)) {
		return false;
	}

	dataLenght = strlen(data);

	for(int i = 0; i < dataLenght; i++) {
		if (data[i] == 10) {
			separatorIndex = i;
			break;
		}

	}

	usernameLengh = separatorIndex + 1;
	passwordLenght = dataLenght - separatorIndex;

	*username = malloc(usernameLengh);
	*password = malloc(passwordLenght);

	for(int i = 0; i < dataLenght; i++) {

		if (i < separatorIndex) {
			(*username)[i] = data[i];
		} else if (i > separatorIndex) {
			(*password)[i - separatorIndex - 1] = data[i];
		}

	}

	(*username)[usernameLengh - 1] = '\0';
	(*password)[passwordLenght - 1] = '\0';

	return true;
}

bool wifi_configured() {
	char *dummy = NULL;
	bool isValid = read_credentials(&dummy) != NULL;
	
	free(dummy);
	
	return isValid;
}

bool update_available_and_ready() {

	nvs_handle_t handle;
	esp_err_t err;
	uint8_t update = 0;

	err = nvs_open("storage", NVS_READWRITE, &handle);
	if (err !=  ESP_OK) {
		nvs_close(handle);
		return false;
	}

	err = nvs_get_u8(handle, kUPDATE_AVAILABLE, &update);

	nvs_close(handle);
	if (err !=  ESP_OK) {
		return false;
	}


	return update == 1;
}

void update_available_and_ready_flag(bool isUpdateable) {

	nvs_handle_t handle;
	esp_err_t err;
	uint8_t value = value = isUpdateable ? 1 : 0;

    err = nvs_open("storage", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return;
    } 

    err = nvs_set_u8(handle, kUPDATE_AVAILABLE, value);
	nvs_close(handle);
}

bool shouldWriteInPrimary() {

	nvs_handle_t handle;
	esp_err_t err;
	uint8_t value = 0;

	err = nvs_open("storage", NVS_READWRITE, &handle);
	if (err !=  ESP_OK) {
		return false;
	}

	err = nvs_get_u8(handle, kNEXT_PARTITION, &value);
	if (err !=  ESP_OK) {
		nvs_close(handle);
		return false;
	}

	nvs_close(handle);
	return (value == PRIMARY_PARTITION_ID);
}

void changePartition() {
	nvs_handle_t handle;
	esp_err_t err;

	bool isPrimary = shouldWriteInPrimary();
	uint8_t nextValue = isPrimary ? SECONDARY_PARTITION_ID : PRIMARY_PARTITION_ID;

    err = nvs_open("storage", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return;
    } 

    err = nvs_set_u8(handle, kNEXT_PARTITION, nextValue);
	if(err != ESP_OK) {
		nvs_close(handle);
		return;
	}

	nvs_close(handle);
}


void write_ota_downloaded(uint8_t value) {
	nvs_handle_t handle;
	esp_err_t err;

	err = nvs_open("storage", NVS_READWRITE, &handle);
	if(err != ESP_OK) {
		return;
	} 

    err = nvs_set_u8(handle, kDOWNLOADED_OTA, value);
	
	nvs_close(handle);
}

void set_ota_downloaded() {
	write_ota_downloaded(1);
}
void clear_ota_downloaded() {
	write_ota_downloaded(0);
}

bool ota_downloaded() {

	nvs_handle_t handle;
	esp_err_t err;
	uint8_t value = 0;

	err = nvs_open("storage", NVS_READWRITE, &handle);
	if (err !=  ESP_OK) {
		return false;
	}

	err = nvs_get_u8(handle, kDOWNLOADED_OTA, &value);
	nvs_close(handle);
	if (err !=  ESP_OK) {
		return false;
	}

	return (value == 1);
}

