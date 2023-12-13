// utils.c

#include "header.h"

int login(const char *username, const char *password) {
    int ok = 0, i, n;
    char useru[30], pasu[30];

    FILE *credentials = fopen(CREDENTIALS_FILE, "r");
    if (credentials == NULL) {
        perror("Eroare open credentials file!");
        exit(EXIT_FAILURE);
    }

    char v_u[10][30];
    char v_p[10][30];

    fscanf(credentials, "%d", &n);
    for (i = 0; i < n; i++) {
        fscanf(credentials, "%s %s", useru, pasu);
        strcpy(v_u[i], useru);
        strcpy(v_p[i], pasu);
    }

    fclose(credentials);

    for (i = 0; i < n; i++) {
        if (strcmp(username, v_u[i]) == 0 && strcmp(password, v_p[i]) == 0) {
            ok = 1;
        }
    }

    return ok;
}

const char *get_file_extension(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (!dot || dot == filename) {
        return "";
    }
    return dot + 1;
}

const char *get_mime_type(const char *file_ext) {
    if (strcasecmp(file_ext, "html") == 0 || strcasecmp(file_ext, "htm") == 0) {
        return HTML;
    } else if (strcasecmp(file_ext, "txt") == 0) {
        return PLAIN;
    } else if (strcasecmp(file_ext, "jpg") == 0 || strcasecmp(file_ext, "jpeg") == 0) {
        return JPEG;
    } else if (strcasecmp(file_ext, "png") == 0) {
        return PNG;
    } else {
        return OTHER;
    }
}

char *url_decode(const char *src) {
    size_t src_len = strlen(src);
    char *decoded = malloc(src_len + 1);

    size_t decoded_len = 0;

    for (size_t i = 0; i < src_len; i++) {
        if (src[i] == '%' && i + 2 < src_len) {
            int hex_val;
            sscanf(src + i + 1, "%2x", &hex_val);
            decoded[decoded_len++] = hex_val;
            i += 2;
        } else {
            decoded[decoded_len++] = src[i];
        }
    }

    decoded[decoded_len] = '\0';
    return decoded;
}

void build_http_response(const char *file_name, const char *file_ext,
                         char *response, size_t *response_len) {
    const char *mime_type = get_mime_type(file_ext);
    char *header = (char *)malloc(BUFFER_SIZE * sizeof(char));
    char *header_err = (char *)malloc(BUFFER_SIZE * sizeof(char));

    snprintf(header, BUFFER_SIZE, "HTTP/1.1 200 OK\r\n"
                                  "Content-Type: %s\r\n"
                                  "\r\n",
             mime_type);

    int file_fd = open(file_name, O_RDONLY);
    if (file_fd == -1) {
        snprintf(header_err, BUFFER_SIZE, "HTTP/1.1 404 Not Found\r\n"
                                          "Content-Type: text/html\r\n"
                                          "\r\n");

        int file_fd_err = open(ERROR_FILE, O_RDONLY);
        if (file_fd_err == -1) {
            perror("Open failed!");
            exit(EXIT_FAILURE);
        }

        struct stat file_err_stat;
        if (fstat(file_fd_err, &file_err_stat)) {
            perror("Bad call!");
            exit(EXIT_FAILURE);
        }
        off_t file_err_size = file_err_stat.st_size;

        *response_len = 0;
        memcpy(response, header_err, strlen(header_err));
        *response_len += strlen(header_err);

        ssize_t bytes_err_read;
        while ((bytes_err_read = read(file_fd_err, response + *response_len, BUFFER_SIZE - *response_len)) > 0) {
            *response_len += bytes_err_read;
        }

        free(header_err);
        close(file_fd_err);
        return;
    }

    struct stat file_stat;
    if (fstat(file_fd, &file_stat)) {
        perror("Bad call!");
        exit(EXIT_FAILURE);
    }
    off_t file_size = file_stat.st_size;

    *response_len = 0;
    memcpy(response, header, strlen(header));
    *response_len += strlen(header);

    ssize_t bytes_read;
    while ((bytes_read = read(file_fd, response + *response_len, BUFFER_SIZE - *response_len)) > 0) {
        *response_len += bytes_read;
    }

    free(header);
    close(file_fd);
}

void *handle_client(void *arg)
{
    Task *task = (Task *)arg;
    int client_fd = task->client_fd;
    free(task);

    char *buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));

    // Primim request-ul de la client și îl stocăm în buffer
    ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
    if (bytes_received > 0)
    {
        // printf("BUFFER PRIMIT: %s\n", buffer);

        // Verificăm dacă avem GET
        regex_t regex;
        regcomp(&regex, "^GET /([^ ]*) HTTP/1", REG_EXTENDED);
        // Expresie regulată
        //  /([^ ]*) calea către un fișier din folder-ul meu
        regmatch_t matches[2];

        if (regexec(&regex, buffer, 2, matches, 0) == 0)
        {
            // Scoatem filename și decodăm URL
            buffer[matches[1].rm_eo] = '\0';
            // printf("BUFFER DUPĂ SET EOF: %s\n", buffer);
            const char *url_encoded_file_name = buffer + matches[1].rm_so;
            // printf("URL ENCODAT: %s\n", url_encoded_file_name);
            char *file_name = url_decode(url_encoded_file_name);
            // printf("FILENAME: %s\n", file_name);

            // Aflăm extensia
            char file_ext[32];
            strcpy(file_ext, get_file_extension(file_name));
            // printf("FILE EXTENSION: %s\n", file_ext);

            // Construim răspunsul HTTP
            char *response = (char *)malloc(BUFFER_SIZE * 2 * sizeof(char));

            size_t response_len;
            build_http_response(file_name, file_ext, response, &response_len);

            // Trimitem răspunsul HTTP la client
            send(client_fd, response, response_len, 0);

            free(response);
            free(file_name);
        }

        regfree(&regex);
    }

    close(client_fd);
    pthread_exit(NULL);

    free(buffer);

    return NULL;
}