// Matew

#include "header.h"

// Structura care reprezintă o sarcină pentru thread-ul din pool
typedef struct
{
    void *(*function)(void *);
    void *argument;
} threadpool_task_t;

// Structura threadpool-ului
typedef struct
{
    threadpool_task_t *queue; // Coada de sarcini
    size_t queue_size;        // Numărul de sarcini în coadă
    size_t queue_capacity;    // Capacitatea maximă a cozi
    pthread_mutex_t lock;     // Mutex pentru protejarea datelor threadpool-ului
    pthread_cond_t not_empty; // Variabilă de condiție - coada nu este goală
    pthread_cond_t not_full;  // Variabilă de condiție - coada nu este plină
    pthread_t *threads;       // Vectorul de thread-uri din pool
    size_t thread_count;      // Numărul total de thread-uri
    int shutdown;             // Indicator dacă threadpool-ul trebuie închis
} threadpool_t;

// Threadpool-ul efectiv
static threadpool_t threadpool;

// Inițializează threadpool-ul cu un număr specific de thread-uri
void initialize_thread_pool(size_t thread_count)
{
    // Inițializează structura threadpool
    threadpool.queue_size = 0;
    threadpool.queue_capacity = thread_count;
    threadpool.queue = malloc(threadpool.queue_capacity * sizeof(threadpool_task_t));
    if (threadpool.queue == NULL)
    {
        perror("Eroare la alocarea memoriei pentru coadă");
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
        perror("Eroare la inițializarea mutex-ului sau a variabilelor de condiție");
        exit(EXIT_FAILURE);
    }

    // Creează thread-urile
    for (size_t i = 0; i < thread_count; ++i)
    {
        if (pthread_create(&threadpool.threads[i], NULL, thread_function, NULL) != 0)
        {
            perror("Eroare la crearea thread-ului");
            exit(EXIT_FAILURE);
        }
    }
}

// Distrugere threadpool
void destroy_thread_pool()
{
    pthread_mutex_lock(&threadpool.lock);
    threadpool.shutdown = 1;
    pthread_mutex_unlock(&threadpool.lock);

    // Așteaptă ca toate thread-urile să termine
    pthread_cond_broadcast(&threadpool.not_empty);

    for (size_t i = 0; i < threadpool.thread_count; ++i)
    {
        if (pthread_join(threadpool.threads[i], NULL) != 0)
        {
            perror("Eroare la așteptarea terminării thread-ului");
            exit(EXIT_FAILURE);
        }
    }

    // Eliberează memoria
    free(threadpool.queue);
    free(threadpool.threads);

    // Distrugerea mutex-ului și a variabilelor de condiție
    pthread_mutex_destroy(&threadpool.lock);
    pthread_cond_destroy(&threadpool.not_empty);
    pthread_cond_destroy(&threadpool.not_full);
}

// Adaugă o sarcină în threadpool
void submit_task(void *(*function)(void *), void *argument)
{
    pthread_mutex_lock(&threadpool.lock);

    // Așteaptă dacă coada este plină
    while (threadpool.queue_size == threadpool.queue_capacity && !threadpool.shutdown)
    {
        printf("Warning: Maximum number of threads reached. Waiting for available slots...\n");
        pthread_cond_wait(&threadpool.not_full, &threadpool.lock);
    }

    // În caz de shutdown, deblochează mutex-ul și iese
    if (threadpool.shutdown)
    {
        pthread_mutex_unlock(&threadpool.lock);
        return;
    }

    // Adaugă sarcina în coadă
    threadpool_task_t task = {function, argument};
    threadpool.queue[threadpool.queue_size++] = task;

    // Semnalează că coada nu mai este goală
    pthread_cond_signal(&threadpool.not_empty);
    pthread_mutex_unlock(&threadpool.lock);
}

// Funcția executată de fiecare thread din pool
void *thread_function(void *arg)
{
    (void)arg; // Evită avertismente despre parametru neutilizat

    while (1)
    {
        pthread_mutex_lock(&threadpool.lock);

        // Așteaptă dacă coada este goală
        while (threadpool.queue_size == 0 && !threadpool.shutdown)
        {
            pthread_cond_wait(&threadpool.not_empty, &threadpool.lock);
        }

        // În caz de shutdown, deblochează mutex-ul și iese
        if (threadpool.shutdown)
        {
            pthread_mutex_unlock(&threadpool.lock);
            pthread_exit(NULL);
        }

        // Extrage o sarcină din coadă
        threadpool_task_t task = threadpool.queue[--threadpool.queue_size];

        // Semnalează că coada nu mai este plină
        pthread_cond_signal(&threadpool.not_full);
        pthread_mutex_unlock(&threadpool.lock);

        // Execută funcția asociată sarcinii
        task.function(task.argument);
    }
}

// Adaugă o sarcină în threadpool specific pentru task-urile de tip handle_client
void add_task_to_thread_pool(Task *task)
{
    pthread_mutex_lock(&threadpool.lock);

    // Așteaptă dacă coada este plină
    while (threadpool.queue_size == threadpool.queue_capacity && !threadpool.shutdown)
    {
        pthread_cond_wait(&threadpool.not_full, &threadpool.lock);
    }

    // În caz de shutdown, deblochează mutex-ul, eliberează memoria și iese
    if (threadpool.shutdown)
    {
        pthread_mutex_unlock(&threadpool.lock);
        free(task);
        return;
    }

    // Adaugă sarcina în coadă
    threadpool_task_t threadpool_task = {handle_client, task}; // Trimite întreaga structură Task
    threadpool.queue[threadpool.queue_size++] = threadpool_task;

    // Semnalează că coada nu mai este goală
    pthread_cond_signal(&threadpool.not_empty);
    pthread_mutex_unlock(&threadpool.lock);
}
