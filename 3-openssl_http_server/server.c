#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <resolv.h>
#include "openssl/ssl.h"
#include "openssl/err.h"
#define FAIL    -1
#define HOST "127.0.0.1"
#define PORT 8888

// Create the SSL socket and intialize the socket address structure
int OpenListener()
{
    int sd;
    struct sockaddr_in addr;
    sd = socket(PF_INET, SOCK_STREAM, 0);
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr(HOST);
    if (bind(sd, (struct sockaddr*)&addr, sizeof(addr)) != 0 )
    {
        perror("can't bind port");
        abort();
    }
    if ( listen(sd, 10) != 0 )
    {
        perror("Can't configure listening port");
        abort();
    }
    printf("\033[92m Server Starting at http://%s:%d\033[0m\n", HOST, PORT);
    return sd;
}

int isRoot()
{
    if (getuid() != 0)
        return 0;
    else
        return 1;
}

SSL_CTX* InitServerCTX(void)
{
    SSL_METHOD *method;
    SSL_CTX *ctx;
    OpenSSL_add_all_algorithms();  /* load & register all cryptos, etc. */
    SSL_load_error_strings();   /* load all error messages */
    method = TLSv1_2_server_method();  /* create new server-method instance */
    ctx = SSL_CTX_new(method);   /* create new context from method */
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

void LoadCertificates(SSL_CTX* ctx, char* CertFile, char* KeyFile)
{
    /* set the local certificate from CertFile */
    if ( SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* set the private key from KeyFile (may be the same as CertFile) */
    if ( SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* verify private key */
    if ( !SSL_CTX_check_private_key(ctx) )
    {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
}

void ShowCerts(SSL* ssl)
{
    X509 *cert;
    char *line;
    cert = SSL_get_peer_certificate(ssl); /* Get certificates (if available) */
    if ( cert != NULL )
    {
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        free(line);
        X509_free(cert);
    }
    else
        printf("No certificates.\n");
}
void Servlet(SSL* ssl) /* Serve the connection -- threadable */
{
    char recv_buffer[1024] = {'\0'};
    int sd, bytes;
    const char* server_response = "Hello, Welcome Login!";
    const char* error_response  = "Username or Password Error!";
    // 合法的用户信息，这里可以存储在 SQLite3 数据库中
    const char *valid_user_info = "admin123456";
    if ( SSL_accept(ssl) == FAIL )     /* do SSL-protocol accept */
        ERR_print_errors_fp(stderr);
    else
    {
        ShowCerts(ssl);        /* get any certificates */
        bytes = SSL_read(ssl, recv_buffer, sizeof(recv_buffer)); /* get request */
        recv_buffer[bytes] = '\0';
        printf("Client msg: \"%s\"\n", recv_buffer);
        if ( bytes > 0 )
        {
            if(strcmp(valid_user_info, recv_buffer) == 0)
                SSL_write(ssl, server_response, strlen(server_response)); /* send reply */
            else
                SSL_write(ssl, error_response, strlen(error_response)); /* send reply */
        }
        else
            ERR_print_errors_fp(stderr);
    }
    sd = SSL_get_fd(ssl);       /* get socket connection */
    SSL_free(ssl);         /* release SSL state */
    close(sd);          /* close connection */
}

int main(int count, char *Argc[])
{
    SSL_CTX *ctx;
    int server;
    char *portnum;
    //Only root user have the permsion to run the server
    if(!isRoot())
    {
        printf("This program must be run as root/sudo user!!");
        exit(0);
    }
    // Initialize the SSL library
    SSL_library_init();
    portnum = Argc[1];
    ctx = InitServerCTX();        /* initialize SSL */
    SSL_CTX_load_verify_locations(ctx, "./client_cert.pem", NULL);  // 信任证书
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);                 // 请求获取客户端证书
    LoadCertificates(ctx, "server_cert.pem", "server_cert.pem");    /* load certs */
    server = OpenListener();    /* create server socket */
    while (1)
    {
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        SSL *ssl;
        int client = accept(server, (struct sockaddr*)&addr, &len);  /* accept connection as usual */
        printf("Connection: %s:%d\n",inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
        ssl = SSL_new(ctx);              /* get new SSL state with context */
        SSL_set_fd(ssl, client);      /* set connection socket to SSL state */
        Servlet(ssl);         /* service connection */
    }
    close(server);          /* close server socket */
    SSL_CTX_free(ctx);         /* release context */
}