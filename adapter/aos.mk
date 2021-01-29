NAME = libamp_adapter

$(NAME)_MBINS_TYPE := kernel
$(NAME)_VERSION := 1.0.0
$(NAME)_SUMMARY := amp adapter

$(NAME)_SOURCES := \
		platform/aos-haas100/peripheral/amp_hal_adc.c \
		platform/aos-haas100/peripheral/amp_hal_dac.c \
		platform/aos-haas100/peripheral/amp_hal_can.c \
		platform/aos-haas100/peripheral/amp_hal_gpio.c \
		platform/aos-haas100/peripheral/amp_hal_i2c.c \
		platform/aos-haas100/peripheral/amp_hal_pwm.c \
		platform/aos-haas100/peripheral/amp_hal_rtc.c \
		platform/aos-haas100/peripheral/amp_hal_spi.c \
		platform/aos-haas100/peripheral/amp_hal_timer.c \
		platform/aos-haas100/peripheral/amp_hal_uart.c \
		platform/aos-haas100/peripheral/amp_hal_wdg.c \
		platform/aos-haas100/peripheral/amp_hal_flash.c \
		platform/aos-haas100/network/amp_tcp.c \
		platform/aos-haas100/network/amp_udp.c \
		platform/aos-haas100/network/amp_dns.c \
		platform/aos-haas100/network/amp_wifi.c \
		platform/aos-haas100/network/amp_cellular.c \
		platform/aos-haas100/network/amp_httpc.c \
		platform/aos-haas100/amp_fs.c \
		platform/aos-haas100/amp_kv.c \
		platform/aos-haas100/amp_ota.c \
		platform/aos-haas100/amp_pm.c \
		platform/aos-haas100/amp_system.c \
		platform/aos-haas100/amp_socket.c \
		platform/aos-haas100/amp_tls_mbedtls.c

$(NAME)_INCLUDES := \
		include \
		include/peripheral \
		platform/aos-haas100 \
		platform/aos-haas100/network \
		../main \
		../components/ota/include \
		../components/linkkit \
		../components/linkkit/infra