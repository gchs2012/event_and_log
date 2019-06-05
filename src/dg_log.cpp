/******************************************************************************

                  Copyright (C), 2017-2018, xxx Co.xxx, Ltd.

 ******************************************************************************
    File Name     : dg_log.c
    Version       : V1.0
    Author        : zc
    Created       : 2018/2/2
    Description   : debug log
    History       :
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/queue.h>

#include "dg_log.h"

#define DG_LOG_FILE_PATH   256
#define DG_LOG_ENTRY_MAX   512

struct dg_log_entry {
    TAILQ_ENTRY(dg_log_entry) next;
    int index;
    int mode;
    long size;
    char file[DG_LOG_FILE_PATH];
};
TAILQ_HEAD(dg_log_head, dg_log_entry);

struct dg_log_free_entry {
    TAILQ_ENTRY(dg_log_free_entry) next;
    int index;
};
TAILQ_HEAD(dg_log_free_head, dg_log_free_entry);

/* Log queue header */
static struct dg_log_head g_dg_log_head =
    TAILQ_HEAD_INITIALIZER(g_dg_log_head);
static struct dg_log_free_head g_dg_log_free_head =
    TAILQ_HEAD_INITIALIZER(g_dg_log_free_head);

/* Total number of log queue nodes */
static struct dg_log_entry g_dg_log_entry[DG_LOG_ENTRY_MAX];
static struct dg_log_free_entry g_dg_log_free_entry[DG_LOG_ENTRY_MAX];

/* Get the head node */
#define DG_LOG_HEAD(_head) (_head) = &g_dg_log_head
#define DG_LOG_FREE_HEAD(_f_head) (_f_head) = &g_dg_log_free_head

/* Get the node by subscript */
#define DG_LOG_ENTRY_BY_INDEX(_elm, _index) (_elm) = &g_dg_log_entry[(_index)]
#define DG_LOG_FREE_ENTRY_BY_INDEX(_var, _index) (_var) = &g_dg_log_free_entry[(_index)]

/* Initialize node */
#define DG_LOG_ENTRY_INIT(_elm) do {  \
    (_elm)->index = -1;               \
    (_elm)->mode = DG_LOG_MODE_FILE;  \
    (_elm)->size = 50 * 1024 * 1024;  \
    (_elm)->file[0] = '\0';           \
} while (0)

typedef struct {
	volatile int locked; /**< lock status 0 = unlocked, 1 = locked */
} dg_log_spinlock_t;

/* Log lock */
static dg_log_spinlock_t g_dg_log_lock;

static volatile int dg_log_init_flag = 0;

/*****************************************************************************
    Prototype    : dg_log_spinlock_lock
    Description  : Lock
    Input        : dg_log_spinlock_t *sl
    Output       : void
    Return Value : void
    Author       : zc
    Date         : 2018/8/2
*****************************************************************************/
static inline void
dg_log_spinlock_lock(dg_log_spinlock_t *sl)
{
	int lock_val = 1;
	asm volatile (
			"1:\n"
			"xchg %[locked], %[lv]\n"
			"test %[lv], %[lv]\n"
			"jz 3f\n"
			"2:\n"
			"pause\n"
			"cmpl $0, %[locked]\n"
			"jnz 2b\n"
			"jmp 1b\n"
			"3:\n"
			: [locked] "=m" (sl->locked), [lv] "=q" (lock_val)
			: "[lv]" (lock_val)
			: "memory");
}

/*****************************************************************************
    Prototype    : dg_log_spinlock_unlock
    Description  : Unlock
    Input        : dg_log_spinlock_t *sl
    Output       : None
    Return Value : None
    Author       : zc
    Date         : 2018/8/2
*****************************************************************************/
static inline void
dg_log_spinlock_unlock(dg_log_spinlock_t *sl)
{
	int unlock_val = 0;
	asm volatile (
			"xchg %[locked], %[ulv]\n"
			: [locked] "=m" (sl->locked), [ulv] "=q" (unlock_val)
			: "[ulv]" (unlock_val)
			: "memory");
}

/*****************************************************************************
    Prototype    : dg_log_init
    Description  : Init
    Input        : const char *file
    Output       : None
    Return Value : int
    Author       : zc
    Date         : 2018/8/2
*****************************************************************************/
int dg_log_init(const char *file)
{
    int i;
    int num  = -1;
    struct dg_log_entry *elm;
    struct dg_log_head *head;
    struct dg_log_free_entry *var;
    struct dg_log_free_head *free_head;

    if (!file) return -1;

    dg_log_spinlock_lock(&g_dg_log_lock);

    DG_LOG_HEAD(head);
    DG_LOG_FREE_HEAD(free_head);

    if (!dg_log_init_flag) {
        for (i = 0; i < DG_LOG_ENTRY_MAX; i++) {
            DG_LOG_ENTRY_BY_INDEX(elm, i);
            DG_LOG_ENTRY_INIT(elm);
            DG_LOG_FREE_ENTRY_BY_INDEX(var, i);
            var->index = i;
            TAILQ_INSERT_TAIL(free_head, var, next);
        }
		dg_log_init_flag = 1;
    }

    TAILQ_FOREACH(elm, head, next) {
        if (!strcmp(elm->file, file)) {
            num = elm->index;
            goto unlock;
        }
    }

    var = TAILQ_FIRST(free_head);
    if (var) {
        DG_LOG_ENTRY_BY_INDEX(elm, var->index);
        elm->index = var->index;
        snprintf(elm->file, sizeof(elm->file), "%s", file);
        TAILQ_INSERT_TAIL(head, elm, next);

        TAILQ_REMOVE(free_head, var, next);
        var->index = -1;
        num = elm->index;
    }

unlock:
    dg_log_spinlock_unlock(&g_dg_log_lock);

    return num;
}

/*****************************************************************************
    Prototype    : dg_log_set_mode
    Description  : Set the log mode
    Input        : int num
                   int mode
    Output       : None
    Return Value : None
    Author       : zc
    Date         : 2018/8/2
*****************************************************************************/
void dg_log_set_mode(int num, int mode)
{
    struct dg_log_entry *elm;
    struct dg_log_head *head;

    if (num < 0) return;

    dg_log_spinlock_lock(&g_dg_log_lock);

    DG_LOG_HEAD(head);

    TAILQ_FOREACH(elm, head, next) {
        if (elm->index == num) {
            elm->mode = mode;
            break;
        }
    }

    dg_log_spinlock_unlock(&g_dg_log_lock);
}

/*****************************************************************************
    Prototype    : dg_log_get_mode
    Description  : Get the log mode
    Input        : int num
    Output       : None
    Return Value : int
    Author       : zc
    Date         : 2018/8/2
*****************************************************************************/
int dg_log_get_mode(int num)
{
    struct dg_log_entry *elm;
    struct dg_log_head *head;
    int mode = DG_LOG_MODE_DISABLE;

    if (num < 0) return mode;

    dg_log_spinlock_lock(&g_dg_log_lock);

    DG_LOG_HEAD(head);

    TAILQ_FOREACH(elm, head, next) {
        if (elm->index == num) {
            mode = elm->mode;
            break;
        }
    }

    dg_log_spinlock_unlock(&g_dg_log_lock);

    return mode;
}

/*****************************************************************************
    Prototype    : dg_log_set_size
    Description  : Set the log file size
    Input        : int num
                   int mode
    Output       : None
    Return Value : None
    Author       : zc
    Date         : 2018/8/2
*****************************************************************************/
void dg_log_set_size(int num, int size)
{
    struct dg_log_entry *elm;
    struct dg_log_head *head;

    if (num < 0) return;

    dg_log_spinlock_lock(&g_dg_log_lock);

    DG_LOG_HEAD(head);

    TAILQ_FOREACH(elm, head, next) {
        if (elm->index == num) {
            if (size >= 50) {
                elm->size = size * 1024 * 1024;
            }
            break;
        }
    }

    dg_log_spinlock_unlock(&g_dg_log_lock);
}

/*****************************************************************************
    Prototype    : dg_log_print
    Description  : Print log
    Input        : int num
                   const char *function
                   unsigned int line
                   const char *format
                   ...
    Output       : None
    Return Value : None
    Author       : zc
    Date         : 2018/8/2
*****************************************************************************/
void dg_log_print(int num, const char *function, unsigned int line,
    const char *format, ...)
{
    if (num < 0) return;

    dg_log_spinlock_lock(&g_dg_log_lock);

    long file_size;
    FILE *fp = stdout;
    struct stat stStat;
    struct dg_log_entry *elm;
    struct dg_log_head *head;

    va_list ap;
    char buf[128];
    char msg[2048];
    time_t tm_t;
    struct tm *ttm = NULL;

    DG_LOG_HEAD(head);

    TAILQ_FOREACH(elm, head, next) {
        if (elm->index == num) {
            break;
        }
    }

    if (elm == NULL) goto unlock;
    if (elm->mode == DG_LOG_MODE_FILE) {
        if (stat(elm->file, &stStat) != 0) {
            if (errno == ENOENT) file_size = 0;
            else goto unlock;
        } else {
            file_size = stStat.st_size;
        }
        if (file_size > elm->size) {
            if ((fp = fopen(elm->file, "w")) == NULL) goto unlock;
        } else {
            if ((fp = fopen(elm->file, "a")) == NULL) goto unlock;
        }
    } else if (elm->mode == DG_LOG_MODE_DISABLE) goto unlock;

    tm_t = time(NULL);
    ttm = localtime(&tm_t);
    strftime(buf, sizeof(buf), "%Y/%m/%d %X", ttm);
    va_start(ap, format);
    vsnprintf(msg, sizeof(msg), format, ap);
    va_end(ap);
    fprintf(fp, "[%s][%s:%d] %s\n", buf, function, line, msg);
    fflush(fp);
    if (fp != stdout) fclose(fp);

unlock:
    dg_log_spinlock_unlock(&g_dg_log_lock);
    return;
}

