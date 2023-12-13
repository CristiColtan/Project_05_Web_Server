// Matew

#include "header.h"

typedef struct
{
    void *(*function)(void *);
    void *argument;
} threadpool_task_t;

typedef struct
{
    threadpool_task_t *queue;
    size_t queue_size;
    size_t queue_capacity;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
    pthread_t *threads;
    size_t thread_count;
    int shutdown;
} threadpool_t;

static threadpool_t threadpool;

void initialize_thread_pool(size_t thread_count)
{
    threadpool.queue_size = 0;
    threadpool.queue_capacity = thread_count;
    threadpool.queue = malloc(threadpool.queue_capacity * sizeof(threadpool_task_t));

    if (threadpool.queue == NULL)
    {
        perror("Eroare la alocarea memoriei pentru coada");
        exit(EXIT_FAILURE);
    }

    threadpool.thread_count = thread_count;
    threadpool.threads = malloc(thread_count * sizeof(pthread_t));
    if (threadpool.threads == NULL)
    {
        perror("Eroare la alocarea memoriei pentru thread-uri");
        exit(EXIT_FAILURE);
    }

    threadpool.shutdown = 0;

    if (pthread_mutex_init(&threadpool.lock, NULL) != 0 ||
        pthread_cond_init(&threadpool.not_empty, NULL) != 0 ||
        pthread_cond_init(&threadpool.not_full, NULL) != 0)
    {
        perror("Eroare la initializarea mutex-ului sau a variabilelor de conditie");
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < thread_count; ++i)
    {
        if (pthread_create(&threadpool.threads[i], NULL, thread_function, NULL) != 0)
        {
            perror("Eroare la crearea thread-ului");
            exit(EXIT_FAILURE);
        }
    }
}

void destroy_thread_pool()
{
    pthread_mutex_lock(&threadpool.lock);
    threadpool.shutdown = 1;
    pthread_mutex_unlock(&threadpool.lock);

    pthread_cond_broadcast(&threadpool.not_empty);

    for (size_t i = 0; i < threadpool.thread_count; ++i)
    {
        if (pthread_join(threadpool.threads[i], NULL) != 0)
        {
            perror("Eroare la așteptarea terminării thread-ului");
            exit(EXIT_FAILURE);
        }
    }

    free(threadpool.queue);
    free(threadpool.threads);

    pthread_mutex_destroy(&threadpool.lock);
    pthread_cond_destroy(&threadpool.not_empty);
    pthread_cond_destroy(&threadpool.not_full);
}

void submit_task(void *(*function)(void *), void *argument)
{
    pthread_mutex_lock(&threadpool.lock);

    while (threadpool.queue_size == threadpool.queue_capacity && !threadpool.shutdown)
    {
        printf("Warning: Maximum number of threads reached. Waiting for available slots...\n");
        pthread_cond_wait(&threadpool.not_full, &threadpool.lock);
    }

    if (threadpool.shutdown)
    {
        pthread_mutex_unlock(&threadpool.lock);
        return;
    }

    threadpool_task_t task = {function, argument};
    threadpool.queue[threadpool.queue_size++] = task;

    pthread_cond_signal(&threadpool.not_empty);
    pthread_mutex_unlock(&threadpool.lock);
}

void *thread_function(void *arg)
{
    (void)arg; // Evită avertismente despre parametru neutilizat

    while (1)
    {
        pthread_mutex_lock(&threadpool.lock);

        while (threadpool.queue_size == 0 && !threadpool.shutdown)
        {
            pthread_cond_wait(&threadpool.not_empty, &threadpool.lock);
        }

        if (threadpool.shutdown)
        {
            pthread_mutex_unlock(&threadpool.lock);
            pthread_exit(NULL);
        }

        threadpool_task_t task = threadpool.queue[--threadpool.queue_size];

        pthread_cond_signal(&threadpool.not_full);
        pthread_mutex_unlock(&threadpool.lock);

        task.function(task.argument);
    }
}

void add_task_to_thread_pool(Task *task)
{
    pthread_mutex_lock(&threadpool.lock);

    while (threadpool.queue_size == threadpool.queue_capacity && !threadpool.shutdown)
    {
        pthread_cond_wait(&threadpool.not_full, &threadpool.lock);
    }

    if (threadpool.shutdown)
    {
        pthread_mutex_unlock(&threadpool.lock);
        free(task); // Elibereaza memoria alocata pentru Task în caz de shutdown
        return;
    }

    threadpool_task_t threadpool_task = {handle_client, task}; // Trimite intreaga structura Task
    threadpool.queue[threadpool.queue_size++] = threadpool_task;

    pthread_cond_signal(&threadpool.not_empty);
    pthread_mutex_unlock(&threadpool.lock);
}
