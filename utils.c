#include "header.h"

// https://github.com/JeffreytheCoder/Simple-HTTP-Server/tree/master
// https://dev.to/jeffreythecoder/how-i-built-a-simple-http-server-from-scratch-using-c-739

const char *get_file_extension(const char *filename)
{
    const char *dot = strrchr(filename, '.');
    // Ultima aparitie a caracterului "."
    // (image.html) -> html
    if (!dot || dot == filename)
    {
        return "";
    }
    return dot + 1;
} // Gasim file extension

const char *get_mime_type(const char *file_ext)
{
    if (strcasecmp(file_ext, "html") == 0 || strcasecmp(file_ext, "htm") == 0)
    {
        return HTML;
    }
    else if (strcasecmp(file_ext, "txt") == 0)
    {
        return PLAIN;
    }
    else if (strcasecmp(file_ext, "jpg") == 0 || strcasecmp(file_ext, "jpeg") == 0)
    {
        return JPEG;
    }
    else if (strcasecmp(file_ext, "png") == 0)
    {
        return PNG;
    }
    else
    {
        return OTHER;
    }
} // Gasim tipul extensiei

char *url_decode(const char *src)
{
    size_t src_len = strlen(src);
    char *decoded = malloc(src_len + 1);

    size_t decoded_len = 0;

    // Decodam %2x -> HEX
    for (size_t i = 0; i < src_len; i++)
    {
        if (src[i] == '%' && i + 2 < src_len)
        {
            int hex_val;
            sscanf(src + i + 1, "%2x", &hex_val);
            // Citim 2 caractere hexa
            decoded[decoded_len++] = hex_val;
            i += 2;
        }
        else
        {
            decoded[decoded_len++] = src[i];
        }
    }

    // Adaugam terminator de sir
    decoded[decoded_len] = '\0';
    return decoded;
}

void build_http_response(const char *file_name, const char *file_ext,
                         char *response, size_t *response_len)
{
    // Cream header-ul HTTP
    const char *mime_type = get_mime_type(file_ext);
    char *header = (char *)malloc(BUFFER_SIZE * sizeof(char));
    char *header_err = (char *)malloc(BUFFER_SIZE * sizeof(char));

    snprintf(header, BUFFER_SIZE, "HTTP/1.1 200 OK\r\n"
                                  "Content-Type: %s\r\n"
                                  "\r\n",
             mime_type);

    // Daca fisierul nu exista, avem 404 Not Found
    int file_fd = open(file_name, O_RDONLY);
    if (file_fd == -1)
    {
        snprintf(header_err, BUFFER_SIZE, "HTTP/1.1 404 Not Found\r\n"
                                          "Content-Type: text/html\r\n"
                                          "\r\n");

        int file_fd_err = open(ERROR_FILE, O_RDONLY);
        if (file_fd_err == -1)
        {
            perror("Open failed!");
            exit(EXIT_FAILURE);
        }

        struct stat file_err_stat;
        if (fstat(file_fd_err, &file_err_stat))
        {
            perror("Bad call!");
            exit(EXIT_FAILURE);
        }
        off_t file_err_size = file_err_stat.st_size;

        *response_len = 0;
        memcpy(response, header_err, strlen(header_err));
        *response_len += strlen(header_err);

        ssize_t bytes_err_read;
        while ((bytes_err_read = read(file_fd_err, response + *response_len, BUFFER_SIZE - *response_len)) > 0)
        {
            *response_len += bytes_err_read;
        }

        free(header_err);
        close(file_fd_err);
        return;
    }

    // Aflam file size-ul
    struct stat file_stat;
    if (fstat(file_fd, &file_stat))
    {
        perror("Bad call!");
        exit(EXIT_FAILURE);
    }
    off_t file_size = file_stat.st_size;

    // Copiem header-ul la raspuns
    *response_len = 0;
    memcpy(response, header, strlen(header));
    *response_len += strlen(header);

    // Copiem file-ul catre response buffer
    ssize_t bytes_read;
    while ((bytes_read = read(file_fd, response + *response_len, BUFFER_SIZE - *response_len)) > 0)
    {
        *response_len += bytes_read;
    }
    free(header);
    close(file_fd);
}

void *handle_client(void *arg)
{
    // printf("Client connection accepted!");
    int client_fd = *((int *)arg);
    char *buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));

    // Primim request-ul de la client si il stocam in buffer
    ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
    if (bytes_received > 0)
    {
        // printf("BUFFER PRIMIT: %s\n", buffer);

        // Verificam daca avem GET
        regex_t regex;
        regcomp(&regex, "^GET /([^ ]*) HTTP/1", REG_EXTENDED);
        // Expresie regulata
        //  /([^ ]*) calea catre un file din folder-ul meu
        regmatch_t matches[2];

        if (regexec(&regex, buffer, 2, matches, 0) == 0)
        {
            // Scoatem filename si decodam URL
            buffer[matches[1].rm_eo] = '\0';
            // printf("BUFFER DUPA SET EOF: %s\n", buffer);
            const char *url_encoded_file_name = buffer + matches[1].rm_so;
            // printf("URL ENCODED: %s\n", url_encoded_file_name);
            char *file_name = url_decode(url_encoded_file_name);
            // printf("FILENAME: %s\n", file_name);

            // Aflam extensia
            char file_ext[32];
            strcpy(file_ext, get_file_extension(file_name));
            // printf("FILE EXTENSION: %s\n", file_ext);

            // Construim raspunsul HTTP
            char *response = (char *)malloc(BUFFER_SIZE * 2 * sizeof(char));

            size_t response_len;
            build_http_response(file_name, file_ext, response, &response_len);

            // Trimitem raspunsul HTTP la client
            send(client_fd, response, response_len, 0);

            free(response);
            free(file_name);
        }

        regfree(&regex);
    }

    close(client_fd);

    free(arg);
    free(buffer);

    return NULL;
}