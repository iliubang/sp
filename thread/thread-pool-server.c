#include <stdio.h>
#define __USE_GNU
#include <pthread.h>
#include <stdlib.h>

#define NUM_HANDLER_THREADS 3

/* global mutex for our program. assignment initializes it. */
/* note that we use a RECURSIVE mutex, since a handler thread might try to lock
 * it twice consecutively. */
pthread_mutex_t request_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

/* global condition variable for our program. assignment initializes it. */
pthread_cond_t got_request = PTHREAD_COND_INITIALIZER;

/* number for pending requests, initially none. */
int num_requests = 0;

/* format of a single request. */
struct request {
    int number;           /* number of the request */
    struct request *next; /* pointer to next request, NULL if none. */
};

/* head of linked list of requests. */
struct request *requests = NULL;
/* pointer to last request */
struct request *last_request = NULL;

void add_request(int request_num, pthread_mutex_t *p_mutex,
                 pthread_cond_t *p_cond_var) {
    /* return code of pthreads functions. */
    int rc;

    /* pointer to newly added request */
    struct request *a_request;

    a_request = (struct request *)malloc(sizeof(struct request));
    if (!a_request) {
        fprintf(stderr, "add_request: out of memory\n");
        exit(1);
    }

    a_request->number = request_num;
    a_request->next = NULL;

    /* lock the mutex, to assure exclusive access to the list */
    rc = pthread_mutex_lock(p_mutex);

    /* add new request to the end of the list, updating list */
    /* pointers as required */
    if (num_requests == 0) {
        requests = a_request;
        last_request = a_request;
    } else {
        last_request->next = a_request;
        last_request = a_request;
    }

    num_requests++;

#ifdef DEBUG
    printf("add_request: added request with id '%d'\n", a_request->number);
    fflush(stdout);
#endif

    /* unlock mutex */
    rc = pthread_mutex_unlock(p_mutex);

    /* signal the condition variable - there's a new request to handle */
    rc = pthread_cond_signal(p_cond_var);
}

struct request *get_request(pthread_mutex_t *p_mutex) {
    int rc;
    struct request *a_request;

    rc = pthread_mutex_lock(p_mutex);

    if (num_requests > 0) {
        a_request = requests;
        requests = a_request->next;

        if (requests == NULL) {
            last_request = NULL;
        }
        num_requests--;
    } else {
        a_request = NULL;
    }

    /* unlock mutex */
    rc = pthread_mutex_unlock(p_mutex);

    return a_request;
}

void handle_request(struct request *a_request, int thread_id) {
    if (a_request) {
        printf("Thread '%d' handled request '%d'\n", thread_id,
               a_request->number);
        fflush(stdout);
    }
}

void handle_requests_loop(void *data) {
    int rc;
    struct request *a_request;
    int thread_id = *((int *)data);

#ifdef DEBUG
    printf("Starting thread '%d'\n", thread_id);
    fflush(stdout);
#endif

    /* lock the mutex, to access the requests list exclusively. */
    rc = pthread_mutex_lock(&request_mutex);

#ifdef DEBUG
    printf("thread '%d' after pthread_mutex_lock\n", thread_id);
    fflush(stdout);
#endif

    /* do forever... */
    while (1) {
#ifdef DEBUG
        printf("thread '%d', number_requests = %d\n", thread_id, num_requests);
        fflush(stdout);
#endif
        if (num_requests > 0) {
            a_request = get_request(&request_mutex);
            if (a_request) {
                handle_request(a_request, thread_id);
                free(a_request);
            }
        } else {
#ifdef DEBUG
            printf("thread '%d' before pthread_cond_wait\n", thread_id);
            fflush(stdout);
#endif
            rc = pthread_cond_wait(&got_request, &request_mutex);

#ifdef DEBUG
            printf("thread '%d' after pthread_cond_wait\n", thread_id);
            fflush(stdout);
#endif
        }
    }
}

int main(int argc, char *argv[]) {
    int i;
    int thr_id[NUM_HANDLER_THREADS];
    pthread_t p_threads[NUM_HANDLER_THREADS];
    struct timespec delay;

    /* create the request-handling threads */
    for (i = 0; i < NUM_HANDLER_THREADS; i++) {
        thr_id[i] = i;
        pthread_create(&p_threads[i], NULL, handle_requests_loop,
                       (void *)&thr_id[i]);
    }

    sleep(3);

    /* run a loop that generates requests */
    for (i = 0; i < 600; i++) {
        add_request(i, &request_mutex, &got_request);
        if (rand() > 3 * (RAND_MAX / 4)) {
            delay.tv_sec = 0;
            delay.tv_nsec = 10;
            nanosleep(&delay, NULL);
        }
    }

    sleep(5);
    printf("Glory, we are done.\n");

    return 0;
}
