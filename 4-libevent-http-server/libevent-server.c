#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     //for getopt, fork
#include <string.h>     //for strcat
//for struct evkeyvalq
#include <sys/queue.h>
#include <event.h>
//for http
//#include <evhttp.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include <event2/http_compat.h>
#include <event2/util.h>
#define MYHTTPD_SIGNATURE   "myhttpd v 0.0.1"
#define HTML_DIR "www"
#define MAX_SIZE 65536
#define MIDDLE_SIZE 1024
#define MIN_SIZE 32

// 响应客户端：ERROR! No found!
void notfound(struct evhttp_request *req, void *arg) {
    //HTTP header
    evhttp_add_header(req->output_headers, "Server", MYHTTPD_SIGNATURE);
    evhttp_add_header(req->output_headers, "Content-Type", "text/html; charset=UTF-8"); // 响应的文件类型
    evhttp_add_header(req->output_headers, "Connection", "close");
    char buffer[MAX_SIZE] = {'\0'};
    sprintf(buffer, "<h1>ERROR! No found!</h1>");
    struct evbuffer *buf;
    buf = evbuffer_new();
    evbuffer_add_printf(buf, "%s\n", buffer); // 发送 HTML 文件
    evhttp_send_reply(req, HTTP_OK, "OK", buf);
    evbuffer_free(buf);
}

// 响应客户端的 GET /index.html 请求
void get_method_handler(struct evhttp_request *req, void *arg) {
    char tmp[1024];
    //获取客户端请求的URI(使用evhttp_request_uri或直接req->uri)
    const char *uri;
    // const char *ht;
    uri = evhttp_request_uri(req);
    // 请求目标 Host
    // ht = evhttp_request_get_host(req);
    printf("GET %s\n", uri);
    /*
       具体的：可以根据GET/POST的参数执行相应操作，然后将结果输出
       ...
     */
    /* 输出到客户端 */
    char filename[MIDDLE_SIZE] = {'\0'}; // 请求的文件名称
    //HTTP header
    evhttp_add_header(req->output_headers, "Server", MYHTTPD_SIGNATURE);
    evhttp_add_header(req->output_headers, "Content-Type", "text/html; charset=UTF-8"); // 响应的文件类型
    evhttp_add_header(req->output_headers, "Connection", "close");

    strcat(filename, HTML_DIR);
    strcat(filename, uri);
    // 发送 HTML 文件
    FILE* fp = fopen(filename, "rb");  // 打开文件
    char html[MAX_SIZE] = {'\0'};
    if (fp != NULL){
        // 获取文件内容
        char data[MAX_SIZE] = {'\0'};
        fgets(data, sizeof(data), fp);
        while (!feof(fp)) {
            strcat(html, data);
            fgets(data, sizeof(data), fp);
        }
    }
    //输出的内容
    struct evbuffer *buf;
    buf = evbuffer_new();
    evbuffer_add_printf(buf, "%s\n", html); // 发送 HTML 文件
    evhttp_send_reply(req, HTTP_OK, "OK", buf);
    evbuffer_free(buf);
}

//当向进程发出SIGTERM/SIGHUP/SIGINT/SIGQUIT的时候，终止event的事件侦听循环
void signal_handler(int sig) {
    switch (sig) {
        case SIGTERM:
        case SIGHUP:
        case SIGQUIT:
        case SIGINT:
            event_loopbreak();  //终止侦听event_dispatch()的事件侦听循环，执行之后的代码
            break;
    }
}
int main(int argc, char *argv[]) {
    //默认参数
    char *httpd_option_listen = "127.0.0.1";
    int httpd_option_port = 8888;
    int httpd_option_daemon = 0;
    int httpd_option_timeout = 120; //in seconds
  
    /* 使用libevent创建HTTP Server */
    //初始化event API
    event_init();
    //创建一个http server
    struct evhttp *httpd;
    httpd = evhttp_start(httpd_option_listen, httpd_option_port);
    printf("Libevent HTTP Server starting at: \033[92m%s:%d\033[0m\n", httpd_option_listen, httpd_option_port);
    evhttp_set_timeout(httpd, httpd_option_timeout);
    // GET请求调用 get_method_handle callback
    evhttp_set_cb(httpd, "/index.html", get_method_handler, NULL);
    // 为未被特定回调捕获的所有请求设置一个回调
	evhttp_set_gencb (httpd, notfound, 0);

    // 循环处理 events
    event_dispatch();
    evhttp_free(httpd);
    return 0;
}
