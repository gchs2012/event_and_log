/******************************************************************************

            版权所有 (C), 2017-2018, xxx Co.xxx, Ltd.

 ******************************************************************************
    文 件 名 : ss_msg.h
    版 本 号 : V1.0
    作    者 : zc
    生成日期 : 2018年8月2日
    功能描述 : 消息结构头文件
    修改历史 :
******************************************************************************/
#ifndef _SS_MSG_H_
#define _SS_MSG_H_

#include "ss_common.h"

#define SS_UNUSED(_x_) (void)(_x_)
#define SS_RETURN(_cond_) \
    if (_cond_) return
#define SS_RETURN_RES(_cond_, _res_) \
    if (_cond_) return _res_
#define SS_GOTO_LABLE(_cond_, _lable_) \
    if (_cond_) goto _lable_

/// 消息结构体
typedef struct ss_msg_s {
    /* Message type */
    int type;
    /* Result of msg processing */
    int result;
    /* Length of segment buffer */
    int len;
    /* Address of segment buffer */
    char data[];
} ss_msg;

typedef int (*ss_msg_pf)(int type, int len, char *data, ss_msg *resp);

/// 消息初始化
int ss_msg_init(int num = 0);

/// 注册函数
void ss_msg_register(ss_msg_pf func, int type);

/// 消息处理线程
void *ss_msg_thread(void *arg);

/// 消息处理
void ss_msg_deal(ss_msg *req, ss_msg *resp);

#endif

