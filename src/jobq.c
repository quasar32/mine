#include "jobq.h"
#include "misc.h"
#include <threads.h>
#include <sys/sysinfo.h>
#include <stdlib.h>

#define MAX_JOBS 1024

#define FATAL(fn, ...) \
    do { \
        if ((fn)(__VA_ARGS__) != thrd_success) { \
            die(#fn " failed\n"); \
        } \
    } while (0)

static cnd_t work_cnd; 
static cnd_t done_cnd;
static mtx_t work_mtx;
static int n_thrds;
static thrd_t *thrds;

struct job {
    task *fn;    
    void *arg;
};

static struct job jobq[MAX_JOBS];
static struct job *head;
static struct job *tail;
static int n_thrds_active;

static void next_job(struct job **ptr) {
    ++*ptr;
    if (*ptr == jobq + MAX_JOBS) {
        *ptr = jobq;
    }
}

static int jobq_do(void *) {
    while (1) {
        mtx_lock(&work_mtx);
        while (!head) {
            cnd_wait(&work_cnd, &work_mtx);
        }
        struct job job;
        job = *head; 
        next_job(&head);
        if (head == tail) {
            head = NULL;
        }
        n_thrds_active++;
        mtx_unlock(&work_mtx);
        job.fn(job.arg);
        mtx_lock(&work_mtx);
        n_thrds_active--;
        if (!n_thrds_active && !head) {
            cnd_signal(&done_cnd);
        }
        mtx_unlock(&work_mtx);
    } 
    return 0;
}

void jobq_new(void) {
    FATAL(cnd_init, &work_cnd);
    FATAL(cnd_init, &done_cnd);
    FATAL(mtx_init, &work_mtx, mtx_plain);
    n_thrds = get_nprocs() / 2; 
    thrds = malloc(n_thrds * sizeof(*thrds));
    for (int i = 0; i < n_thrds; i++) {
        FATAL(thrd_create, &thrds[i], jobq_do, NULL);
    }
}

void jobq_add(task *fn, void *arg) {
    mtx_lock(&work_mtx);  
    if (!head) {
        head = jobq;
        tail = jobq;
    } else if (head == tail) {
        die("too many jobs\n");
    }
    tail->fn = fn;
    tail->arg = arg;
    next_job(&tail);
    cnd_broadcast(&work_cnd);
    mtx_unlock(&work_mtx);
}

void jobq_wait(void) {
    mtx_lock(&work_mtx);
    while (head || n_thrds_active) {
        cnd_wait(&done_cnd, &work_mtx);
    }
    mtx_unlock(&work_mtx);
}
