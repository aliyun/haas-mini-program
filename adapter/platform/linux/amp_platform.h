/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdarg.h>
#include <unistd.h>
#include <memory.h>
#include <inttypes.h>
#include <errno.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <mqueue.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/prctl.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <arpa/nameser.h>

#include <netinet/tcp.h>
#include <netdb.h>
#include <resolv.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef AMP_UI_SUPPORT
#include <SDL2/SDL.h>
#include "aui_drivers/display/monitor.h"
#include "aui_drivers/indev/mouse.h"
#include "aui_drivers/indev/mousewheel.h"
#include "aui_drivers/indev/keyboard.h"
#endif