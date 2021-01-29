NAME = libamp_services

$(NAME)_MBINS_TYPE := kernel
$(NAME)_VERSION := 1.0.0
$(NAME)_SUMMARY := amp component services

$(NAME)_SOURCES += \
		app_mgr/app_mgr.c \
		app_mgr/app_ntp.c \
		app_mgr/app_management_center.c \
		app_mgr/ota_socket.c

$(NAME)_SOURCES += \
		board_mgr/board_info.c \
		board_mgr/board_mgr.c

$(NAME)_SOURCES += \
		recovery/recovery.c \
		recovery/ymodem.c

$(NAME)_SOURCES += \
		amp_utils/amp_utils.c \

$(NAME)_INCLUDES := \
		../main \
		../adapter/include \
		../adapter/include/peripheral \
		../adapter/platform/aos-haas100 \
		../utils/mbedtls/include \
		../utils/cJSON \
		../components/linkkit \
		../components/linkkit/infra \
		../components/linkkit/dev_model \
		../components/linkkit/dev_sign \
		../components/linkkit/mqtt \
		../components/ota/include \
		../components/ulog \
		../components/das/include \
		amp_utils \
		app_mgr \
		board_mgr \
		recovery

