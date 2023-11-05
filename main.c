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

const char *get_file_extension(const char *filename){
    const char *dot = strrchr(filename,'.'); 
    //Ultima aparitie a caracterului "." (image.html)
    if(!dot || dot == filename){
        return "";
    }
    return dot + 1;
}   //Gasim extensia

const char *get_mime_type(const char *file_ext){
    if (strcasecmp(file_ext,"html")==0 || strcasecmp(file_ext,"htm")==0){
        return "text/html";
    }
    else if (strcasecmp(file_ext,"txt")==0){
        return "text/plain";
    }
    else if (strcasecmp(file_ext,"jpg")==0 || strcasecmp(file_ext,"jpeg")==0){
        return "image/jpeg";
    }
    else if (strcasecmp(file_ext,"png")==0){
        return "image/png";
    }
    else{
        return "application/octet-stream";
    }
}   //Gasim tipul extensiei

bool case_insensitive_compare(const char *s1, const char *s2){
    while (*s1 && *s2){
        if (tolower((unsigned char)*s1) != tolower((unsigned)*s2)){
            return false;    
        }
        s1++;
        s2++;
    }
    return *s1 == *s2;
}   //NU NE TREBUIE MOMENTAN

char *get_file_case_insensitive(const char *file_name){
    //https://www.ibm.com/docs/en/zos/2.4.0?topic=functions-opendir-open-directory
    DIR *dir = opendir(".");
    if (dir == NULL){
        perror("opendir");
        return NULL;
    }

    struct dirent *entry;
    char *found_file_name = NULL;
    while ((entry = readdir(dir)) != NULL){
        if (case_insensitive_compare(entry->d_name, file_name)){
            found_file_name=entry->d_name;
            break;
        }
    }
    
    closedir(dir);
    return found_file_name;
}   //NU NE TREBUIE MOMENTAN

char *url_decode(const char *src){
    size_t src_len = strlen(src);
    char *decoded = malloc (src_len + 1);
    size_t decoded_len = 0;

    //Decodam %2x -> HEX
    for (size_t i = 0; i < src_len; i++){
        if (src[i] == '%' && i + 2 < src_len){
            int hex_val;
            sscanf(src+i+1, "%2x", &hex_val);
            //Citim 2 caractere hexa
            decoded[decoded_len++]=hex_val;
            //decoded_len++;
            i+=2;
        }
        else{
            decoded[decoded_len++]=src[i];
        }
    }

    //Adaugam terminator de sir
    decoded[decoded_len]='\0';
    return decoded;
}

void build_http_response(const char *file_name, const char *file_ext,
char *response, size_t *response_len){
    //Cream header-ul HTTP
    const char *mime_type = get_mime_type(file_ext);
    char *header = (char*)malloc(BUFFER_SIZE * sizeof(char));
    snprintf(header, BUFFER_SIZE, "HTTP/1.1 200 OK\r\n"
                                  "Content-Type: %s\r\n"
                                  "\r\n",mime_type);
    
    //Daca fisierul nu exista, avem 404 Not Found
    int file_fd = open(file_name, O_RDONLY);
    if(file_fd == -1){
        snprintf(response, BUFFER_SIZE,"HTTP/1.1 404 Not Found\r\n"
                                       "Content-Type: text/html\r\n"
                                       "\r\n"
                                       "<!DOCTYPE html>\r\n"
                                       "<html lang=\"en\">\r\n"
                                       "<head>\r\n"
                                       "<meta charset=\"UTF-8\">\r\n"
                    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\r\n"
                                       "<title>NOT FOUND</title>\r\n"
                                       "</head>\r\n"
                                       "<body>\r\n"
                                       "<h1>ERROR 404 !!!</h1>\r\n"
                                       "<h1>NOT FOUND :(</h1>\r\n"
                                       "<img src=\"error.jpg\" alt=\"Error Image\"/>\r\n"
                                       "</body>\r\n"
                                       "</html>\r\n");
        *response_len = strlen(response);
        return;
    }

    //Aflam file size-ul
    struct stat file_stat;
    if(fstat(file_fd, &file_stat)){
        perror("Bad call!");
        exit(EXIT_FAILURE);
    }
    off_t file_size = file_stat.st_size;

    //Copiem header-ul la raspuns
    *response_len=0;
    memcpy(response,header, strlen(header));
    *response_len += strlen(header);

    //Copiem file-ul catre response buffer
    ssize_t bytes_read;
    while((bytes_read = read (file_fd, response + *response_len,
    BUFFER_SIZE - *response_len)) > 0){
        *response_len += bytes_read;
    }
    free(header);
    close(file_fd);
}

void *handle_client(void *arg){
    //printf("Client connection accepted!");
    int client_fd = *((int*)arg);
    char *buffer = (char*)malloc(BUFFER_SIZE * sizeof(char));

    //Primim request-ul de la client si il stocam in buffer
    ssize_t bytes_received= recv(client_fd, buffer, BUFFER_SIZE, 0);
    if(bytes_received > 0){
    //Verificam daca avem GET
        regex_t regex;
        regcomp(&regex, "^GET /([^ ]*) HTTP/1", REG_EXTENDED);
        //Expresie regulata
        // /([^ ]*) calea catre un file din folder-ul meu
        regmatch_t matches[2];

        if(regexec(&regex, buffer, 2, matches, 0)==0){
    //Scoatem filename si decodam URL
            buffer[matches[1].rm_eo]='\0';
            const char *url_encoded_file_name = buffer +
        matches[1].rm_so;
            char *file_name = url_decode(url_encoded_file_name);
    //Aflam extensia
            char file_ext[32];
            strcpy(file_ext, get_file_extension(file_name));

    //Construim raspunsul HTTP
            char *response = (char*)malloc(BUFFER_SIZE*2*sizeof(char));
            size_t response_len;
            build_http_response(file_name,file_ext,response,&response_len);

    //Trimitem raspunsul HTTP la client
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

//SETUP <><><><><><><><><><><><><><><><><><><><><><><><> //
//Folosim perror (good practice <3)
int main(int argc, char* argv[]){
    int server_fd;
    struct sockaddr_in server_addr;

    //Create socket.
//AF_INET -> IPv4;
//SOCK_STREAM -> TCP (garanteaza livrarea);
//INADDR_ANY -> Orice conexiune e acceptata.

    if((server_fd= socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Socket fail!");
        exit(EXIT_FAILURE);
    }//Daca avem valoarea 0 se alege un protocol automat.
     //Daca valoarea IF ului e -1 avem eroare.

    //Configure socket.
    server_addr.sin_family=AF_INET;
    server_addr.sin_addr.s_addr=INADDR_ANY;
    server_addr.sin_port=htons(8080);

    //printf("Socket configured!\n");

    //Bind socket to port 8080.
    if (bind(server_fd, (struct sockaddr*)&server_addr,sizeof(server_addr)) < 0){
        perror ("Bind fail!");
        exit(EXIT_FAILURE);
    }

    //printf("Socket bind successful!\n");

    //Listen
    if (listen(server_fd, 10) < 0){
        perror("Listen fail!");
        exit (EXIT_FAILURE);
    }//Ascultam maxim 10 in acelasi timp.

    printf("Listening on port 8080\n");
    while(1){//Ascultam la infinit pentru noi clienti
        //Client info
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int* client_fd = malloc(sizeof(int));

        //Accept client connection
        if ((*client_fd = accept(server_fd, (struct sockaddr*)& client_addr, &client_addr_len)) < 0){
            perror("Accept failed!");
            continue;
        }

        //printf("Client connection accepted!");

        //Create thread to handle client request
        pthread_t thread_id;

        //TO DO:
        pthread_create(&thread_id, NULL, handle_client, (void*)client_fd);
        //Functia cere void* ==> void *handle_client()
        pthread_detach(thread_id);
    }

    close(server_fd);
    return 0;
}