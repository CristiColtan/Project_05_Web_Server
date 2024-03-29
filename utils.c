// Coltz

#include "header.h"

// https://github.com/JeffreytheCoder/Simple-HTTP-Server/tree/master
// https://dev.to/jeffreythecoder/how-i-built-a-simple-http-server-from-scratch-using-c-739

int login(const char *username, const char *password)
{
    int ok = 0, i, n;
    char useru[30], pasu[30];

    FILE *credentials = fopen(CREDENTIALS_FILE, "r");
    if (credentials == NULL)
    {
        perror("Eroare open credentials file!");
        exit(EXIT_FAILURE);
    }

    char v_u[10][30];
    char v_p[10][30];

    fscanf(credentials, "%d", &n);
    for (i = 0; i < n; i++)
    {
        fscanf(credentials, "%s %s", useru, pasu);
        strcpy(v_u[i], useru);
        strcpy(v_p[i], pasu);
    }

    fclose(credentials);

    for (i = 0; i < n; i++)
    {
        if (strcmp(username, v_u[i]) == 0 && strcmp(password, v_p[i]) == 0)
        {
            ok = 1;
        }
    }

    return ok;
}

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
    else if (strcasecmp(file_ext, "php") == 0)
    {
        return PHP;
    }
    else if (strcasecmp(file_ext, "js") == 0)
    {
        return JAVA;
    }
    else
    {
        return OTHER;
        // Nu stie cum sa interpreteze si download-eaza fisier-ul
        // Feature folosit la multe servere
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

void build_http_ok(const char *file_name, const char *file_ext, char *response, size_t *response_len)
{
    char *header = (char *)malloc(BUFFER_SIZE * sizeof(char));
    const char *mime_type = get_mime_type(file_ext);

    snprintf(header, BUFFER_SIZE, "HTTP/1.1 200 OK\r\n"
                                  "Content-Type: %s\r\n"
                                  "\r\n",
             mime_type);

    int file_fd = open(file_name, O_RDONLY);
    if (file_fd == -1)
    {
        perror("Open failed!");
        exit(EXIT_FAILURE);
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

void build_http_error(const char *file_name, char *response, size_t *response_len)
{
    char *header_err = (char *)malloc(BUFFER_SIZE * sizeof(char));

    snprintf(header_err, BUFFER_SIZE, ERROR_HEADER);

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
}

void build_http_response(const char *file_name, const char *file_ext,
                         char *response, size_t *response_len)
{
    // Daca fisierul nu exista, avem 404 Not Found
    int file_fd = open(file_name, O_RDONLY);
    if (file_fd == -1)
    {
        build_http_error(file_name, response, response_len);
        return;
    }

    // Cream header-ul HTTP
    if (strcmp(file_ext, "php") == 0)
    {
        build_http_ok("test.php", file_ext, response, response_len);
        return;
    }
    if (strcmp(file_ext, "js") == 0)
    {
        build_http_ok("testjava.txt", "txt", response, response_len);
        return;
    }
    build_http_ok(file_name, file_ext, response, response_len);
}

void *handle_client(void *arg)
{
    Task *task = (Task *)arg;
    int client_fd = task->client_fd;
    free(task);

    char *buffer = (char *)malloc(BUFFER_SIZE * sizeof(char));

    // Primim request-ul de la client si il stocam in buffer
    ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
    if (bytes_received > 0)
    {

        // Verificam daca avem GET / POST / PUT
        regex_t regex_get, regex_post, regex_put;
        regcomp(&regex_get, "^GET /([^ ]*) HTTP/1", REG_EXTENDED);
        regcomp(&regex_post, "^POST /([^ ]*) HTTP/1", REG_EXTENDED);
        regcomp(&regex_put, "^PUT /([^ ]*) HTTP/1", REG_EXTENDED);

        // Expresie regulata
        //  /([^ ]*) calea catre un file din folder-ul meu
        regmatch_t get_matches[2],
            post_matches[2],
            put_matches[2];

        if (regexec(&regex_get, buffer, 2, get_matches, 0) == 0)
        {
            // Scoatem filename si decodam URL
            buffer[get_matches[1].rm_eo] = '\0';

            const char *url_encoded_file_name = buffer + get_matches[1].rm_so;

            char *file_name = url_decode(url_encoded_file_name);
            
            // Aflam extensia
            char file_ext[32];
            strcpy(file_ext, get_file_extension(file_name));

            if (strcmp(file_ext, "php") == 0)
            {
                printf("\nPHP FILE! Interpreting it!...\n");
                interpretPHP(file_name);
            }

            printf("\nExtensie: %s", file_ext);

            if (strcmp(file_ext, "js") == 0)
            {
                printf("\nJAVASCRIPT FILE! Interpreting it!...\n");
                interpretJAVA(file_name);
            }

            // Construim raspunsul HTTP
            char *response = (char *)malloc(BUFFER_SIZE * 2 * sizeof(char));

            size_t response_len;
            build_http_response(file_name, file_ext, response, &response_len);

            // Trimitem raspunsul HTTP la client
            send(client_fd, response, response_len, 0);

            free(response);
            free(file_name);
        }

        else if (regexec(&regex_post, buffer, 2, post_matches, 0) == 0)
        {
            // Cerere POST
            process_post_request(buffer);
        }
        else if (regexec(&regex_put, buffer, 2, put_matches, 0) == 0)
        {
            // Cerere PUT
            process_put_request(buffer);
        }

        regfree(&regex_get);
        regfree(&regex_post);
        regfree(&regex_put);
    }

    close(client_fd);
    pthread_exit(NULL);

    free(arg);
    free(buffer);

    return NULL;
}

void process_post_request(const char *buffer)
{
    // Găsește începutul datelor POST în buffer
    const char *post_data_start = strstr(buffer, "\r\n\r\n");
    if (post_data_start != NULL)
    {
        post_data_start += 4;

        printf("Data POST primită:\n%s\n", post_data_start);
    }
}

void process_put_request(const char *buffer)
{
    // Găsește începutul datelor PUT în buffer
    const char *put_data_start = strstr(buffer, "\r\n\r\n");
    if (put_data_start != NULL)
    {
        put_data_start += 4;

        printf("Data PUT primită:\n%s\n", put_data_start);
    }
}
