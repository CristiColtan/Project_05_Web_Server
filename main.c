#include "header.h"

size_t thread_number = 0;

// SETUP <><><><><><><><><><><><><><><><><><><><><><><><> //
// Folosim perror (good practice <3)
int main(int argc, char *argv[])
// https://youtu.be/gk6NL1pZi1M?si=8-WP40uOXXblo1GA
// https://youtu.be/cEH_ipqHbUw?si=5epmWXYcK2o0zBoN
{
    printf("Welcome!\n");
    // Open config file;
    FILE *config_file;
    config_file = fopen(CONFIG_FILE, "r");
    // Var1- thread_number;
    // Var2-
    if (config_file == NULL)
    {
        perror("Eroare open config file!");
        exit(EXIT_FAILURE);
    }

    fscanf(config_file, "%zu", &thread_number);
    fclose(config_file);
    // printf("THREADS: %d\n", thread_number);

    // Initialize thread pool
    initialize_thread_pool(thread_number);
    printf("App configured successful!\n");

    char user[30];
    char pass[30];
    printf("Username: ");
    scanf("%s", user);
    printf("Password: ");
    scanf("%s", pass);

    int ok = login(user, pass);
    if (ok == 1)
    {
        printf("Login succesful!\n");
    }
    else
    {
        printf("Access denied!");
        return 1;
    }

    int server_fd;
    struct sockaddr_in server_addr;

    // Create socket.
    // AF_INET -> IPv4;
    // SOCK_STREAM -> TCP (garanteaza livrarea);
    // INADDR_ANY -> Orice conexiune e acceptata.

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket fail!");
        exit(EXIT_FAILURE);
    } // Daca avem valoarea 0 se alege un protocol automat.
      // Daca valoarea IF ului e -1 avem eroare.

    // Configure socket.
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    // printf("Socket configured!\n");

    // Bind socket to port 8080.
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind fail!");
        exit(EXIT_FAILURE);
    }

    // printf("Socket bind successful!\n");

    // Listen
    if (listen(server_fd, 10) < 0)
    {
        perror("Listen fail!");
        exit(EXIT_FAILURE);
    } // Ascultam maxim 10 in acelasi timp.

    size_t total_clients = 0;
    printf("Listening on port 8080\n");
    while (1)
    { // Ascultam la infinit pentru noi clienti
        // Client info
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int *client_fd = malloc(sizeof(int));

        // Accept client connection
        if ((*client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len)) < 0)
        {
            perror("Accept failed!");
            continue;
        }

        // printf("Client connection accepted!");

        Task *task = malloc(sizeof(Task));
        if (task == NULL)
        {
            perror("Malloc fail!");
            exit(EXIT_FAILURE);
        }

        task->client_fd = *client_fd;

        // Create thread to handle client request
        add_task_to_thread_pool(task);
        // Functia cere void* ==> void *handle_client()
    }

    destroy_thread_pool();
    close(server_fd);
    return 0;
}