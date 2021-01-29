TARGET = libota.a
MODULE = ota

MOD_SOURCES := \
		ota_agent/ota_service.c

MOD_SOURCES += \
		ota_agent/download/ota_download_http.c \
		ota_agent/download/ota_download_file2fs_http.c \
		ota_agent/transport/ota_transport_mqtt.c \
		ota_agent/verify/ota_verify_hash.c \
		ota_agent/verify/ota_verify_rsa.c \
		ota_agent/tools/upack_data_file.c

MOD_SOURCES += \
		hal/ota_hal_ctrl.c   \
        hal/ota_hal_digest.c \
		hal/ota_hal_os.c     \
		hal/ota_hal_param.c  \
		hal/ota_hal_trans.c  \
		hal/ota_hal_plat.c   \
		hal/ota_hal_fs_ctrl.c \
		hal/ota_hal_fs_plat.c

MOD_INCLUDES := \
		. \
		include \
		hal     \
		ota_agent/tools \
		../ulog \
		../../adapter/include \
		../../adapter/include/peripheral \
		../../adapter/platform/linux \
		../../utils/mbedtls/include \
		../../utils/cJSON \
		../http/include  \
		../linkkit/mqtt  \
		../linkkit/infra \
		../linkkit

include $(TOOLS_DIR)/rules.mk
