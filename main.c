#include "header.h"

int main(int argc, char *argv[]) {
    // Deschide config file
    FILE *config_file;
    config_file = fopen(CONFIG_FILE, "r");
    if (config_file == NULL) {
        perror("Eroare open config file!");
        exit(EXIT_FAILURE);
    }

    size_t num_threads;
    fscanf(config_file, "%zu", &num_threads);
    fclose(config_file);

    // Inițializează thread pool-ul cu numărul de thread-uri citit din config
    initialize_thread_pool(num_threads);

    printf("Welcome!\n");
    printf("App configured successful!\n");

    char user[30];
    char pass[30];
    printf("Username: ");
    scanf("%s", user);
    printf("Password: ");
    scanf("%s", pass);

    int ok = login(user, pass);
    if (ok == 1) {
        printf("Login successful!\n");
    } else {
        printf("Access denied!\n");
        return 1;
    }

    int server_fd;
    struct sockaddr_in server_addr;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket fail!");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind fail!");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("Listen fail!");
        exit(EXIT_FAILURE);
    }

    printf("Listening on port 8080\n");

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        int *client_fd = malloc(sizeof(int));
        *client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);

        if (*client_fd < 0) {
            perror("Accept failed!");
            continue;
        }

        // Crează un Task și adaugă-l în threadpool
        Task *task = malloc(sizeof(Task));
        task->client_fd = *client_fd;
        if (task == NULL) {
            perror("Eroare la alocarea memoriei pentru Task");
            exit(EXIT_FAILURE);
        }

        add_task_to_thread_pool(task);
    }

    // Nu uita să distrugi threadpool-ul la finalul programului
    destroy_thread_pool();

    close(server_fd);
    return 0;
}
