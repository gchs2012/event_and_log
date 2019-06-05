# 注册日志接口
## 1、消息初始化
```
int ss_msg_init(int num = 0)
```

## 2、消息注册
``` void ss_msg_register(ss_msg_pf func, int type) ```

## 3、消息处理线程（根据具体情况而使用）
``` void *ss_msg_thread(void *arg) ```

## 4、消息处理
void ss_msg_deal(ss_msg *req, ss_msg *resp)

## 5、消息类型添加(ss_common.h)
```
/// 消息类型
enum {
    SS_MSG_TYPE_MIN = 0,

    // <-------------- 新增消息类型 -------------->
    ......
    // <---------------------------------------->

#define SS_RESP_RESULT_INFO       "Response message"
    SS_RESP_RESULT_TYPE,

    SS_MSG_TYPE_MAX,
};

/// 通过类型获取信息
#define SS_GET_MSG_INFO(_type_) ({ \
    const char *_msg_ = NULL; \
    switch (_type_) { \
        case SS_TSL_LOG_ENABLE_TYPE:  (_msg_) = SS_TSL_LOG_ENABLE_INFO; break;  \
        case SS_TSL_LOG_DISABLE_TYPE: (_msg_) = SS_TSL_LOG_DISABLE_INFO; break; \
        case SS_RESP_RESULT_TYPE:     (_msg_) = SS_RESP_RESULT_INFO; break;     \
    } \
    _msg_; })
```
