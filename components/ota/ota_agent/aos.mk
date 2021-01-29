NAME := libamp_ota_agent

$(NAME)_MBINS_TYPE := kernel
$(NAME)_VERSION := 1.0.1
$(NAME)_SUMMARY := An over-the-air update is the wireless delivery of new software. 

#default gcc
ifeq ($(COMPILER),)
$(NAME)_CFLAGS      += -Wall -Werror
else ifeq ($(COMPILER),gcc)
$(NAME)_CFLAGS      += -Wall -Werror
endif

#GLOBAL_INCLUDES += include
$(NAME)_INCLUDES += include \
                    verify \
                    mcu \
                    tools \
                    tools/xz/include \
                    ../../../adapter/include \
                    ../../../adapter/include/peripheral \
                    ../../../adapter/platform/aos-haas100 \
		              ../../../utils/cJSON \
		              ../../http/include  \
		              ../../linkkit/mqtt  \
                    ../../linkkit \
                    ../../linkkit/infra
                    

ifneq (,$(filter breeze,$(COMPONENTS)))
GLOBAL_DEFINES += OTA_CONFIG_BLE
$(NAME)_SOURCES := download/ota_download_ble.c   \
                   transport/ota_transport_ble.c

else
$(NAME)_SOURCES := download/ota_download_http.c   \
                   download/ota_download_file2fs_http.c \
                   transport/ota_transport_mqtt.c

#$(NAME)_COMPONENTS += http
endif

ifeq ($(OTA_CONFIG_SLAVE),y)
$(NAME)_SOURCES := mcu/ota_slaver_upgrade.c
else
$(NAME)_SOURCES += ota_service.c            \
                   verify/ota_verify_hash.c \
                   verify/ota_verify_rsa.c
endif

ifeq (,$(filter mcu_linuximpl,$(HOST_MCU_FAMILY)))
$(NAME)_SOURCES += tools/upack_data_file.c
endif

#$(NAME)_COMPONENTS += cjson

ifeq ($(USE_ITLS),y)
   $(NAME)_COMPONENTS += itls
   ifeq ($(COMPILER),armcc)
   GLOBAL_CFLAGS += -DOTA_CONFIG_ITLS -DCONFIG_HTTP_SECURE_ITLS=1 -DOTA_SIGNAL_CHANNEL=1 -DCONFIG_HTTP_SECURE=1
   else
   GLOBAL_DEFINES += OTA_CONFIG_ITLS CONFIG_HTTP_SECURE_ITLS=1 OTA_SIGNAL_CHANNEL=1 CONFIG_HTTP_SECURE=1
   endif
else
   ifeq ($(OTA_CONFIG_SLAVE),y)
   GLOBAL_DEFINES += OTA_SIGNAL_CHANNEL=3
   else
      $(NAME)_COMPONENTS += mbedtls
      ifeq ($(COMPILER),armcc)
         ifeq ($(HTTPS_DL),1)
         GLOBAL_CFLAGS += -DOTA_CONFIG_SECURE_DL_MODE
         endif
         ifneq (,$(filter mcu_esp8266,$(HOST_MCU_FAMILY)))
         GLOBAL_CFLAGS += -DOTA_SIGNAL_CHANNEL=1 -DCONFIG_HTTP_SECURE=1 -DOTA_CONFIG_SECURE_DL_MODE
         else
         GLOBAL_CFLAGS += -DCONFIG_HTTP_SECURE=1
         endif
      else
         ifeq ($(HTTPS_DL),1)
         GLOBAL_DEFINES += OTA_CONFIG_SECURE_DL_MODE
         endif
         ifneq (,$(filter mcu_esp8266,$(HOST_MCU_FAMILY)))
         GLOBAL_DEFINES += OTA_SIGNAL_CHANNEL=1 CONFIG_HTTP_SECURE=1 OTA_CONFIG_SECURE_DL_MODE
         else
         GLOBAL_DEFINES += OTA_SIGNAL_CHANNEL=1 CONFIG_HTTP_SECURE=1
         endif
      endif
   endif
endif
