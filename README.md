# 注册日志接口
## 1、消息初始化
int ss_msg_init(int num = 0)

## 2、消息注册
void ss_msg_register(ss_msg_pf func, int type)

## 3、消息处理线程（根据具体情况而使用）
void *ss_msg_thread(void *arg)

## 4、消息处理
void ss_msg_deal(ss_msg *req, ss_msg *resp)

## 5、消息类型添加(ss_common.h)
