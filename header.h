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
#define OTHER "application/octet-stream"

#define CONFIG_FILE "config.txt"
#define CREDENTIALS_FILE "credentials.txt"
#define ERROR_FILE "error.html"
#define ERROR_HEADER ""
#define REGG "^GET /([^ ]*) HTTP/1"

const char *get_file_extension(const char *filename);

const char *get_mime_type(const char *file_ext);

char *url_decode(const char *src);

void build_http_response(const char *file_name, const char *file_ext,
                         char *response, size_t *response_len);

void *handle_client(void *arg);

int login(const char *username, const char *password);

#endif