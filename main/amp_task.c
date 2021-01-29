/*
 * Copyright (C) 2015-2020 Alibaba Group Holding Limited
 */

#include "amp_platform.h"
#include "amp_defines.h"
#include "amp_system.h"
#include "infra_list.h"
#include "amp_task.h"

#define MOD_STR                 "AMP_TASK"
#define AMP_MSGQ_WAITIME        (2000)
#define AMP_MSGQ_MAX_NUM        10

static amp_osMessageQId amp_task_mq = NULL; /* JSEngine message queue */
static void *amp_task_mutex     = NULL;     /* JSEngine mutex */

typedef struct timer_link{
    amp_osTimerId timer_id;
    dlist_t node;
} timer_link_t;

static dlist_t g_timer_list = LIST_HEAD_INIT(g_timer_list);

extern void jsengine_exit();
extern void hw_timer_clear(void);

void timer_list_clear(void)
{
    dlist_t *temp;
    timer_link_t *timer_node;

    dlist_for_each_entry_safe(&g_timer_list, temp, timer_node, timer_link_t, node)
    {
        amp_timer_stop(timer_node->timer_id);
        amp_timer_delete(timer_node->timer_id);
        dlist_del(&timer_node->node);
        amp_free(timer_node);
    }

    hw_timer_clear();
}

int32_t amp_task_yield(uint32_t timeout)
{
    int32_t ret = 0;
    amp_task_msg_t amp_msg;

    memset(&amp_msg, 0, sizeof(amp_msg));
    amp_msg.type = AMP_TASK_MSG_TYPE_MAX;
    
    if ((ret = amp_queue_recv(amp_task_mq, &amp_msg, sizeof(amp_task_msg_t), timeout)) != 0) {
        return -1;
    }

    if (amp_msg.type == AMP_TASK_MSG_CALLBACK) {
        amp_msg.callback(amp_msg.param);
    }

    else if(amp_msg.type == AMP_TASK_MSG_EXIT) {
        return 1;
    }

    return 0;
}

static void amp_task_timer_cb_handler(void *arg)
{
    amp_task_msg_t *p_amp_msg = (amp_task_msg_t *)amp_get_timer_params(arg);

    if (amp_task_mq == NULL) {
        return;
    }
        
    amp_queue_send(amp_task_mq, p_amp_msg, sizeof(amp_task_msg_t), 0);
}

void *amp_task_timer_action(uint32_t ms, amp_engine_call_t action, void *arg,
                            amp_timer_type_t type)
{
    amp_osTimerId timer_id = NULL;

    amp_task_msg_t *p_param =
        (amp_task_msg_t *)amp_calloc(1, sizeof(amp_task_msg_t));

    if (!p_param) return NULL;

    if (amp_task_mq == NULL) {
        goto fail;
    }

    p_param->callback = action;
    p_param->param    = arg;
    p_param->type     = AMP_TASK_MSG_CALLBACK;

    if (type == AMP_TIMER_REPEAT) {
        timer_id = amp_timer_create("amp_task_timer", amp_task_timer_cb_handler,
                                    p_param);
    } else if (type == AMP_TIMER_ONCE) {
        timer_id = amp_timer_create("amp_task_timer", amp_task_timer_cb_handler,
                                         p_param);
    } else {
        goto fail;
    }

    if (!timer_id) goto fail;

    int ret = amp_timer_start(timer_id, ms, type);
    if (ret) {
        amp_timer_delete(timer_id);
        goto fail;
    }

    //add timer id to link
    timer_link_t *timer_link = amp_malloc(sizeof(timer_link_t));
    timer_link->timer_id = timer_id;
    dlist_add_tail(&timer_link->node, &g_timer_list);

    return timer_id;

fail:
    amp_free(p_param);
    return NULL;
}

int32_t amp_task_cancel_timer(void *timerid)
{
    if (!timerid) return -1;

    timer_link_t *timer_node;

    dlist_for_each_entry(&g_timer_list, timer_node, timer_link_t, node)
    {
        if (timer_node->timer_id != timerid) continue;
        dlist_del(&timer_node->node);
        amp_free(timer_node);
    }

    amp_timer_stop((amp_osTimerId)timerid);
    return amp_timer_delete((amp_osTimerId)timerid);
}

int32_t amp_task_schedule_call(amp_engine_call_t call, void *arg)
{
    amp_task_msg_t msg_buf;
    amp_task_msg_t *p_param = &msg_buf;

    if (amp_task_mq == NULL) {
        return -1;
    }

    p_param->callback = call;
    p_param->param    = arg;
    p_param->type     = AMP_TASK_MSG_CALLBACK;
    if (amp_task_mq == NULL) {
        amp_warn(MOD_STR, "amp_task_mq has not been initlized");
        return -1;
    }
    amp_queue_send(amp_task_mq, p_param, sizeof(amp_task_msg_t), 0);
    return 0;
}

int32_t amp_task_exit_call(amp_engine_call_t call, void *arg)
{
    amp_task_msg_t msg_buf;
    amp_task_msg_t *p_param = &msg_buf;

    memset(p_param, 0, sizeof(amp_task_msg_t));
    p_param->callback = call;
    p_param->param    = arg;
    p_param->type     = AMP_TASK_MSG_EXIT;
    if (amp_task_mq == NULL) {
        amp_warn(MOD_STR, "amp_task_mq has not been initlized");
        return -1;
    }
    amp_mutex_lock(amp_task_mutex);
    amp_queue_send(amp_task_mq, p_param, sizeof(amp_task_msg_t), AMP_MSGQ_WAITIME);
    amp_mutex_unlock(amp_task_mutex);
    return 0;
}

int32_t amp_task_init()
{
    if (amp_task_mq != NULL) {
        return 0;
    }

    if ((amp_task_mq = amp_queue_create(
             AMP_MSGQ_MAX_NUM, sizeof(amp_task_msg_t))) == NULL) {
        amp_error(MOD_STR, "create messageQ error");
        return -1;
    }
    if ((amp_task_mutex = amp_mutex_create()) == NULL) {
        amp_error(MOD_STR, "create mutex error");
        return -1;
    }

    amp_debug(MOD_STR, "jsengine task init");
    return 0;
}

int32_t amp_task_deinit()
{   
    if (amp_task_mq != NULL) {
        amp_queue_delete(amp_task_mq);
        amp_task_mq = NULL;
    }

    if (amp_task_mutex != NULL) {
        amp_mutex_destroy(amp_task_mutex);
        amp_task_mutex = NULL;
    }

    /* free all jsengine heap */
    jsengine_exit();

    amp_debug(MOD_STR, "jsengine task free");
    return 0;
}


