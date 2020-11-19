#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#define FAIL    -1
#define HOST "127.0.0.1"
#define PORT 8888

int OpenConnection()
{
    int sfd;
    struct hostent *host;
    struct sockaddr_in addr;
    sfd = socket(PF_INET, SOCK_STREAM, 0);
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr(HOST);
    if ( connect(sfd, (struct sockaddr*)&addr, sizeof(addr)) != 0 )
    {
        close(sfd);
        perror(HOST);
        abort();
    }
    return sfd;
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

SSL_CTX* InitCTX(void)
{
    SSL_METHOD *method;
    SSL_CTX *ctx;
    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */
    method = TLSv1_2_client_method();  /* Create new client-method instance */
    ctx = SSL_CTX_new(method);   /* Create new context */
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
}

// 服务器证书信息
void ShowCerts(SSL* ssl)
{
    X509 *cert;
    char *line;
    cert = SSL_get_peer_certificate(ssl); /* get the server's certificate */
    if ( cert != NULL )
    {
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);       /* free the malloc'ed string */
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        free(line);       /* free the malloc'ed string */
        X509_free(cert);     /* free the malloc'ed certificate copy */
    }
    else
        printf("Info: No client certificates configured.\n");
}

// 客户端实现用户登陆
int main()
{
    SSL_CTX *ctx;
    int server;
    SSL *ssl;
    char recv_buffer[1024] = {'\0'};
    char send_buffer[1024] = {'\0'};
    int bytes;
    SSL_library_init();
    ctx = InitCTX();
    LoadCertificates(ctx, "client_cert.pem", "client_cert.pem"); /* load certs */
    server = OpenConnection(); // 创建套接字，连接服务器
    ssl = SSL_new(ctx);      /* create new SSL connection state */
    SSL_set_fd(ssl, server);    /* attach the socket descriptor */
    if ( SSL_connect(ssl) == FAIL )   /* perform the connection */
        ERR_print_errors_fp(stderr);
    else
    {
        char user_name[16] = {'\0'};
        char password[16] = {'\0'};
        const char *send_msg = "%s%s";
        printf("Enter the User Name : ");
        scanf("%s", user_name);
        printf("\nEnter the Password : ");
        scanf("%s", password);
        sprintf(send_buffer, send_msg, user_name, password);   /* construct reply */
        printf("\n\nConnected with %s encryption\n", SSL_get_cipher(ssl));
        ShowCerts(ssl);        /* get any certs */
        SSL_write(ssl, send_buffer, strlen(send_buffer));   /* encrypt & send message */
        bytes = SSL_read(ssl, recv_buffer, sizeof(recv_buffer)); /* get reply & decrypt */
        recv_buffer[bytes] = 0;
        printf("Received:\033[92m\"%s\"\033[0m\n", recv_buffer);
        SSL_free(ssl);        /* release connection state */
    }
    close(server);            /* close socket */
    SSL_CTX_free(ctx);        /* release context */
    return 0;
}
