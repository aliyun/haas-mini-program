/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#include "amp_platform.h"
#include "amp_defines.h"
#include "amp_utils.h"
#include "amp_config.h"
#include "amp_system.h"
#include "amp_network.h"
#include "amp_kv.h"
#include "amp_fs.h"
#include "recovery.h"
#ifdef JSE_ADVANCED_ADDON_UI
#include "render_public.h"
#include "aui_conf.h"

extern volatile int app_run_flag;

volatile int g_ui_run_flag = 1;
#endif

#define MOD_STR "AMP_MAIN"

extern void jsengine_main(void);

void *jsengine_func(void *argv)
{
    jsengine_main();

    amp_thread_delete(NULL);

    return NULL;
}

int amp_main(void)
{
    char version[AMP_VERSION_LENGTH] = {0};
    char *dev_name = NULL;
    amp_version_get(version);
    /* init ulog module */
    amp_printf("=================amp info=================\n");
    amp_printf("amp version: %s\n", version);
    amp_printf("amp build time: %s,%s\n", __DATE__, __TIME__);
    amp_printf("==========================================\n");
    dev_name = (char *)amp_get_device_name();
    if(dev_name != NULL) {
        amp_printf("dev_name = %s\r\n", dev_name);
    }
    /* system init */
    amp_system_init();

    /* init ulog module */
    ulog_init();

    /* set ulog level, make all the level of log is not lower than this value could be logged */
    aos_set_log_level(AOS_LL_ERROR);
    
    int ret = -1;

    void *jsengine_task;
    int jsengine_stack_used;
    amp_os_thread_param_t jsengine_task_params = {0};

#ifdef AMP_KV_ENABLE
    ret = kv_init();
    if (ret != 0) {
        amp_warn(MOD_STR, "kv init failed!");
    }
#endif

    jsengine_task_params.name = "amp_jsengine";
    jsengine_task_params.priority = amp_get_default_task_priority();
    jsengine_task_params.stack_size = 1024 * 8;

    /* amp main start */
    ret = amp_thread_create(&jsengine_task, jsengine_func, NULL, &jsengine_task_params, &jsengine_stack_used);
    if (ret != 0) {
        amp_debug(MOD_STR, "jsengine task creat failed!");
        return ret;
    }

#ifdef JSE_ADVANCED_ADDON_UI
    while(!app_run_flag){
        amp_msleep(50);
    }
	
    if (g_ui_run_flag) {
        amp_view_model_init();
        render_init();
	}
#endif
    while(1) {
        amp_msleep(5000);
    }

    return 0;
}

