#include <event2/util.h>
#include <event2/buffer.h>
#include <event2/http.h>

// 初始化 event API
struct event_base *event_init(void);
// 在 address:port 上启动 HTTP 服务器
struct evhttp *evhttp_start(const char *address, ev_uint16_t port);
// 设置 HTTP 请求超时时间
void evhttp_set_timeout(struct evhttp *http, int timeout_in_secs);
// 设置 HTTP 服务器允许的方法，默认允许：GET, POST, HEAD, PUT, DELETE
void evhttp_set_allowed_methods(struct evhttp* http, ev_uint16_t methods);
// 设置一个 URI 的回调函数
int evhttp_set_cb(struct evhttp *http, const char *path,
    void (*cb)(struct evhttp_request *, void *), void *cb_arg);
// 删除一个 URI 的回调函数
int evhttp_del_cb(struct evhttp *, const char *);
// 为未被特定回调捕获的所有请求设置一个回调
void evhttp_set_gencb(struct evhttp *http,
    void (*cb)(struct evhttp_request *, void *), void *arg);
// 将添加构造 HTML Header
int evhttp_add_header(struct evkeyvalq *headers, const char *key, const char *value);
// 为新的 evbuffer 分配存储空间
struct evbuffer *evbuffer_new(void);
// 释放 buffer 空间
void evbuffer_free(struct evbuffer *buf);
// 将字符串追加到 buf
int evbuffer_add_printf(struct evbuffer *buf, const char *fmt, ...);
// 发送一个 HTML 响应客户端
void evhttp_send_reply(struct evhttp_request *req, int code,
    const char *reason, struct evbuffer *databuf);
// 循环处理事件 events
int event_dispatch(void);
// 释放前面创建的 HTTP 服务器
void evhttp_free(struct evhttp* http);
// # chunk 分块传输
// 初始化响应回复，Transfer-Encoding chunked.
void evhttp_send_reply_start(struct evhttp_request *req, int code,
    const char *reason);
void evhttp_send_reply_chunk(struct evhttp_request *req,
    struct evbuffer *databuf);
void evhttp_send_reply_chunk_with_cb(struct evhttp_request *, struct evbuffer *,
    void (*cb)(struct evhttp_connection *, void *), void *arg);
void evhttp_send_reply_end(struct evhttp_request *req);


