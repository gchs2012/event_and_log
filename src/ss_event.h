/******************************************************************************

            版权所有 (C), 2017-2018, xxx Co.xxx, Ltd.

 ******************************************************************************
    文 件 名 : ss_event.h
    版 本 号 : V1.0
    作    者 : zc
    生成日期 : 2018年8月2日
    功能描述 : 事件处理头文件
    修改历史 :
******************************************************************************/
#ifndef _SS_EVENT_H_
#define _SS_EVENT_H_

#include <sys/epoll.h>

#define SS_UNIX_SOCKET   "/var/ss_event_%d.sock"
#define SS_EVENT_MAX     500 + 128
#define SS_BUFER_MAX     1400

#define SS_NONE         0
#define SS_READABLE     1
#define SS_WRITABLE     2
#define SS_ALL_EVENTS   (SS_READABLE|SS_WRITABLE)

struct ss_event_loop_s;
typedef void (*ss_event_proc)(
    struct ss_event_loop_s *eventLoop, int fd, int mask, void *privdata);

/// 客户端信息结构体
typedef struct ss_client_s {
    int fd;
    int wlen;
    char rbuf[SS_BUFER_MAX];
    char wbuf[SS_BUFER_MAX];
} ss_client;

/// 事件结构体
typedef struct ss_file_event_s {
    int mask;
    void *clientdata;
    ss_event_proc reventProc;
    ss_event_proc weventProc;
} ss_file_event;

/// A fired event
typedef struct ss_fired_event_s {
    int fd;
    int mask;
} ss_fired_event;

/// 事件结构体
typedef struct ss_event_loop_s {
    int sfd;
    int epfd;
    int maxfd;
    int setsize;
    ss_client client[SS_EVENT_MAX];
    ss_file_event events[SS_EVENT_MAX];
    ss_fired_event fired[SS_EVENT_MAX];
    struct epoll_event ep_events[SS_EVENT_MAX];
} ss_event_loop;

#endif

