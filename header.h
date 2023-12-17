#ifndef HEADER_H
#define HEADER_H

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

#include <unistd.h>
#include <dirent.h>
#include <regex.h>
#include <pthread.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>

#include <arpa/inet.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/types.h>

#define BUFFER_SIZE (104857600)

#define HTML "text/html"
#define PLAIN "text/plain"
#define JPEG "image/jpeg"
#define PNG "image/png"
#define PHP "text/html"
#define JAVA "text/html"
#define OTHER "application/octet-stream"

#define CONFIG_FILE "config.txt"
#define CREDENTIALS_FILE "credentials.txt"
#define ERROR_FILE "error.html"
#define ERROR_HEADER "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n"
#define REGG "^GET /([^ ]*) HTTP/1"

// THREADPOOL
typedef struct
{
    void (*task_function)(int);
    int client_fd;
} Task;

// HTTP RESPONSES
#define HTTP_CONTINUE "HTTP/1.1 100 Continue"
#define HTTP_OK "HTTP/1.1 200 OK"
#define HTTP_BAD_REQUEST "HTTP/1.1 400 Bad Request"
#define HTTP_NOT_FOUND "HTTP/1.1 404 Not Found"
#define HTTP_LARGE_PAYLOAD "HTTP/1.1 413 Payload Too Large"
#define HTTP_LONG_URI "HTTP/1.1 414 URI Too Long"
#define HTTP_MEDIA "HTTP/1.1 415 Unsupported Media Type"
#define HTTP_JOKE "HTTP/1.1 418 I'm a Teapot"
//

// SERVER
const char *get_file_extension(const char *filename);

const char *get_mime_type(const char *file_ext);

char *url_decode(const char *src);

void build_http_response(const char *file_name, const char *file_ext,
                         char *response, size_t *response_len);

void *handle_client(void *arg);

int login(const char *username, const char *password);

// THREADS
void initialize_thread_pool(size_t thread_count);

void destroy_thread_pool();

void submit_task(void *(*function)(void *), void *argument);

void *thread_function(void *arg);

void add_task_to_thread_pool(Task *task);

// OTHERS
void interpretPHP(const char *file_name);

void interpretJAVA(const char *file_name);

void build_http_error(const char *file_name, char *response, size_t *response_len);

void build_http_ok(const char *file_name, const char *file_ext, char *response, size_t *response_len);

void process_post_request(const char *buffer);

void process_put_request(const char *buffer);
#endif