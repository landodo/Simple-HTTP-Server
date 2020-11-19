// DServer
// GET/POST 文件上传和文件下载
#include <stdio.h>
#include <stdlib.h>       // exit
#include <arpa/inet.h>    // inet_ntoa
#include <netinet/in.h>   // sockaddr_in
#include <string.h>       // strlen
#include <unistd.h>       // close function
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <netinet/tcp.h>

#include "config.h"

int start_server();
void handle_request(int cfd, char uri[]);
void construct_header(char *header, int status, char *type);
void get_filetype(char *filename, char *filetype);
const char *get_status_by_code(int status);
void handle_post(int cfd, char buffer[]);
void request_image(int cfd, char uri[]);

int main() {

  int sfd = start_server();

  while (1) {
    // 4. 服务器接收客户端的连接请求
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    int cfd = accept(sfd, (struct sockaddr *)&client_addr, &len);
    if (cfd == -1) {
      perror("accept() error!\n");
      exit(EXIT_FAILURE);
    }
    // DEBUG: 连接成功，打印出客户端的 IP 和端口号
    // printf("client ip: %s, port: %d\n", inet_ntoa(client_addr.sin_addr), htons(client_addr.sin_port));
    
    char buffer[MAX_SIZE] = {'\0'};
    ssize_t rbytes = recv(cfd, buffer, sizeof(buffer), 0);
    if (rbytes == -1) {
      perror("recv() error!\n");
      exit(EXIT_FAILURE);
    }
    printf("%s\n", buffer);  // 请求内容

    char method[MIN_SIZE] = {'\0'};
    char uri[MIDDLE_SIZE] = {'\0'};
    sscanf(buffer, "%s %s", method, uri);

    // GET Method
    if (!strcmp(method, "GET")) {
      handle_request(cfd, uri);
    }

    // POST Method
    else if (!strcmp(method, "POST")) {
      handle_post(cfd, buffer);
    }

    else {
      printf("Method Not implemented!\n");
    }
    close(cfd);
  }
  close(sfd);
  return 0;
}

// handle_post 上传文件
void handle_post(int cfd, char buffer[]) {
  // 写入文件
  FILE *f = fopen("save.txt", "wb");
  fwrite(buffer, sizeof(char), strlen(buffer), f);
  fclose(f);
  
  char buf[MAX_SIZE] = {'\0'};
  strcpy(buf, "HTTP/1.0 200 OK\r\n");
  send(cfd, buf, strlen(buf), 0);
  strcpy(buf, SERVER_STRING);
  send(cfd, buf, strlen(buf), 0);
  sprintf(buf, "Content-Type: text/html\r\n");
  send(cfd, buf, strlen(buf), 0);
  strcpy(buf, "\r\n");
  send(cfd, buf, strlen(buf), 0);
  
  char html[MIDDLE_SIZE];
  char line[MIDDLE_SIZE];
  char filename[MIN_SIZE];
  sprintf(filename, "%s%s", HTML_DIR, LOADED_FILE);
  FILE* fp = fopen(filename, "rb");
  if (fp != NULL){
    char data[MAX_SIZE] = {'\0'};
    
    fgets(data, sizeof(data), fp);
    while (!feof(fp)) {
      send(cfd, data, strlen(data), 0);
      fgets(data, sizeof(data), fp);
    }
  }
}

// 图片显示异常，先不管图片了。
void handle_request(int cfd, char uri[]) {
  char filename[MIDDLE_SIZE] = {'\0'};
  if (!strcmp(uri, "/")) {
     sprintf(filename, "%s%s", HTML_DIR, DEFAULT_FILE); // www/index.html
  } else {
    sprintf(filename, "%s%s", HTML_DIR, uri+1);
  }
  
  // 构造 HTTP header
  char header[MAX_SIZE] = {'\0'};
  char file_type[MIN_SIZE] = {'\0'};
  get_filetype(filename, file_type);
  construct_header(header, 200, file_type);
  send(cfd, header, strlen(header), 0);
  printf("%s\n", header);

  // 打开文件进行传输
  FILE* fp = fopen(filename, "r");
  if (fp != NULL){
    char data[MAX_SIZE] = {'\0'};
    
    fgets(data, sizeof(data), fp);
    while (!feof(fp)) {
      send(cfd, data, strlen(data), 0);
      fgets(data, sizeof(data), fp);
    }
    fclose(fp);
  }
}

int start_server() {
  // 1. 创建 socket
  int sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd < 0) {
    printf("socket error:%s\n",strerror(errno));
    exit(-1);
  }

  // 2. bind() 绑定 IP 和端口号
  struct sockaddr_in server_addr;  //用于填写服务器的ip地址与端口号
  server_addr.sin_family = AF_INET;  //地址族
  server_addr.sin_port = htons(PORT);  //端口号
  server_addr.sin_addr.s_addr = inet_addr(HOST);  // IP 地址
  if (bind(sfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("bind()");
    exit(EXIT_FAILURE);
  }

  // 3. 建立监听队列
  if (listen(sfd, 10) < 0) {
    perror("listen()");
    exit(EXIT_FAILURE);
  }

  // 输出提示信息
  printf("\033[92m DServer Starting at http://%s:%d\033[0m\n",HOST, PORT);
  return sfd;
}

void get_filetype(char *filename, char *filetype) {
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");

  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");

  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");

  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");

  else if(strstr(filename, ".css"))
    strcpy(filetype, "text/css");
  else
    strcpy(filetype, "text/plain");
}

void construct_header(char *header, int status, char *type)
{
    const char *msg = get_status_by_code(status);
    sprintf(header, "HTTP/1.1 %d %s\r\n", status, msg);
    sprintf(header, "%sContent-Type:%s\r\n", header, type);
    sprintf(header, "%sServer:DServer\r\n", header);
    if(status < 400)
        sprintf(header, "%sConnection: keep-alive\r\n", header);
    sprintf(header, "%s\r\n", header);
}

const char *get_status_by_code(int status)
{
    switch (status)
    {
    // 1×× Informational
    case 100:
        return "Continue";
    case 101:
        return "Switching Protocols";
    case 102:
        return "Processing";
    // 2×× Success
    case 200:
        return "OK";
    case 201:
        return "Created";
    case 202:
        return "Accepted";
    case 203:
        return "Non-authoritative Information";
    case 204:
        return "No Content";
    case 205:
        return "Reset Content";
    case 206:
        return "Partial Content";
    case 207:
        return "Multi-Status";
    case 208:
        return "Already Reported";
    case 226:
        return "IM Used";
    // 3×× Redirection
    case 300:
        return "Multiple Choices";
    case 301:
        return "Moved Permanently";
    case 302:
        return "Found";
    case 303:
        return "See Other";
    case 304:
        return "Not Modified";
    case 305:
        return "Use Proxy";
    case 307:
        return "Temporary Redirect";
    case 308:
        return "Permanent Redirect";
    // 4×× Client Error
    case 400:
        return "Bad Request";
    case 401:
        return "Unauthorized";
    case 402:
        return "Payment Required";
    case 403:
        return "Forbidden";
    case 404:
        return "Not Found";
    case 405:
        return "Method Not Allowed";
    case 406:
        return "Not Acceptable";
    case 407:
        return "Proxy Authentication Required";
    case 408:
        return "Request Timeout";
    case 409:
        return "Conflict";
    case 410:
        return "Gone";
    case 411:
        return "Length Required";
    case 412:
        return "Precondition Failed";
    case 413:
        return "Payload Too Large";
    case 414:
        return "Request-URI Too Long";
    case 415:
        return "Unsupported Media Type";
    case 416:
        return "Requested Range Not Satisfiable";
    case 417:
        return "Expectation Failed";
    case 418:
        return "I'm a teapot";
    case 421:
        return "Misdirected Request";
    case 422:
        return "Unprocessable Entity";
    case 423:
        return "Locked";
    case 424:
        return "Failed Dependency";
    case 426:
        return "Upgrade Required";
    case 428:
        return "Precondition Required";
    case 429:
        return "Too Many Requests";
    case 431:
        return "Request Header Fields Too Large";
    case 444:
        return "Connection Closed Without Response";
    case 451:
        return "Unavailable For Legal Reasons";
    case 499:
        return "Client Closed Request";
    // 5×× Server Error
    case 500:
        return "Internal Server Error";
    case 501:
        return "Not Implemented";
    case 502:
        return "Bad Gateway";
    case 503:
        return "Service Unavailable";
    case 504:
        return "Gateway Timeout";
    case 505:
        return "HTTP Version Not Supported";
    case 506:
        return "Variant Also Negotiates";
    case 507:
        return "Insufficient Storage";
    case 508:
        return "Loop Detected";
    case 510:
        return "Not Extended";
    case 511:
        return "Network Authentication Required";
    case 599:
        return "Network Connect Timeout Error";
    default:
        return "Unknown HTTP status code";
    }
}