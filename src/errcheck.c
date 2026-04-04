/* ============================================================================
 * errcheck.c
 * Implementations of checked POSIX wrappers declared in errcheck.h.
 *
 * Each wrapper calls the underlying system function and, on failure, prints
 * a diagnostic message (including PID, source file, and line number) before
 * terminating.  Thread-related wrappers call pthread_exit() instead of
 * exit() so only the faulting thread is killed rather than the whole process.
 * ============================================================================ */

#include "../headers/errcheck.h"

/* --------------------------------------------------------------------------
 * Process termination helpers
 * -------------------------------------------------------------------------- */

/* Terminate the process, printing `msg` and the errno description if set. */
void termina(const char *msg) {
    if (errno == 0)
        fprintf(stderr, "== %d == %s\n", getpid(), msg);
    else
        fprintf(stderr, "== %d == %s: %s\n", getpid(), msg, strerror(errno));
    exit(1);
}

/* Same as termina() but also reports source file and line number. */
void xtermina(const char *msg, int line, char *file) {
    if (errno == 0)
        fprintf(stderr, "== %d == %s\n", getpid(), msg);
    else
        fprintf(stderr, "== %d == %s: %s\n", getpid(), msg, strerror(errno));
    fprintf(stderr, "== %d == Line: %d, File: %s\n", getpid(), line, file);
    exit(1);
}

/* --------------------------------------------------------------------------
 * FILE * operations
 * -------------------------------------------------------------------------- */

FILE *xfopen(const char *path, const char *mode, int line, char *file) {
    FILE *f = fopen(path, mode);
    if (f == NULL) {
        perror("fopen failed");
        fprintf(stderr, "== %d == Line: %d, File: %s\n", getpid(), line, file);
        exit(1);
    }
    return f;
}

/* --------------------------------------------------------------------------
 * File descriptor operations
 * -------------------------------------------------------------------------- */

void xclose(int fd, int line, char *file) {
    int e = close(fd);
    if (e != 0) {
        perror("close failed");
        fprintf(stderr, "== %d == Line: %d, File: %s\n", getpid(), line, file);
        exit(1);
    }
}

/* --------------------------------------------------------------------------
 * Process operations
 * -------------------------------------------------------------------------- */

pid_t xfork(int line, char *file) {
    pid_t p = fork();
    if (p < 0) {
        perror("fork failed");
        fprintf(stderr, "== %d == Line: %d, File: %s\n", getpid(), line, file);
        exit(1);
    }
    return p;
}

pid_t xwait(int *status, int line, char *file) {
    pid_t p = wait(status);
    if (p < 0) {
        perror("wait failed");
        fprintf(stderr, "== %d == Line: %d, File: %s\n", getpid(), line, file);
        exit(1);
    }
    return p;
}

/* --------------------------------------------------------------------------
 * Pipe operations
 * -------------------------------------------------------------------------- */

int xpipe(int pipefd[2], int line, char *file) {
    int e = pipe(pipefd);
    if (e != 0) {
        perror("pipe failed");
        fprintf(stderr, "== %d == Line: %d, File: %s\n", getpid(), line, file);
        exit(1);
    }
    return e;
}

/* --------------------------------------------------------------------------
 * POSIX shared memory
 * -------------------------------------------------------------------------- */

int xshm_open(const char *name, int oflag, mode_t mode, int line, char *file) {
    int e = shm_open(name, oflag, mode);
    if (e == -1) {
        perror("shm_open failed");
        fprintf(stderr, "== %d == Line: %d, File: %s\n", getpid(), line, file);
        exit(1);
    }
    return e;
}

int xshm_unlink(const char *name, int line, char *file) {
    int e = shm_unlink(name);
    if (e == -1) {
        perror("shm_unlink failed");
        fprintf(stderr, "== %d == Line: %d, File: %s\n", getpid(), line, file);
        exit(1);
    }
    return e;
}

int xftruncate(int fd, off_t length, int line, char *file) {
    int e = ftruncate(fd, length);
    if (e == -1) {
        perror("ftruncate failed");
        fprintf(stderr, "== %d == Line: %d, File: %s\n", getpid(), line, file);
        exit(1);
    }
    return e;
}

void *simple_mmap(size_t length, int fd, int line, char *file) {
    void *a = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (a == (void *)-1) {
        perror("mmap failed");
        fprintf(stderr, "== %d == Line: %d, File: %s\n", getpid(), line, file);
        exit(1);
    }
    return a;
}

int xmunmap(void *addr, size_t length, int line, char *file) {
    int e = munmap(addr, length);
    if (e == -1) {
        perror("munmap failed");
        fprintf(stderr, "== %d == Line: %d, File: %s\n", getpid(), line, file);
        exit(1);
    }
    return e;
}

/* --------------------------------------------------------------------------
 * POSIX semaphores — named
 * --------------------------------------------------------------------------
 * NOTE: On error, the thread wrappers below call pthread_exit() rather than
 * exit() because exit() would tear down all threads in the process.  POSIX
 * semaphores are used by both threads and processes; these wrappers are
 * written for a thread context. */

sem_t *xsem_open(const char *name, int oflag, mode_t mode,
                 unsigned int value, int line, char *file) {
    sem_t *s = sem_open(name, oflag, mode, value);
    if (s == SEM_FAILED) {
        perror("sem_open failed");
        fprintf(stderr, "== %d == Line: %d, File: %s\n", getpid(), line, file);
        exit(1);
    }
    return s;
}

int xsem_close(sem_t *s, int line, char *file) {
    int e = sem_close(s);
    if (e != 0) {
        perror("sem_close failed");
        fprintf(stderr, "== %d == Line: %d, File: %s\n", getpid(), line, file);
        exit(1);
    }
    return e;
}

int xsem_unlink(const char *name, int line, char *file) {
    int e = sem_unlink(name);
    if (e != 0) {
        perror("sem_unlink failed");
        fprintf(stderr, "== %d == Line: %d, File: %s\n", getpid(), line, file);
        exit(1);
    }
    return e;
}

/* POSIX semaphores — unnamed */

int xsem_init(sem_t *sem, int pshared, unsigned int value,
              int line, char *file) {
    int e = sem_init(sem, pshared, value);
    if (e != 0) {
        perror("sem_init failed");
        fprintf(stderr, "== %d == Line: %d, File: %s\n", getpid(), line, file);
        exit(1);
    }
    return e;
}

int xsem_destroy(sem_t *sem, int line, char *file) {
    int e = sem_destroy(sem);
    if (e != 0) {
        perror("sem_destroy failed");
        fprintf(stderr, "== %d == Line: %d, File: %s\n", getpid(), line, file);
        exit(1);
    }
    return e;
}

/* POSIX semaphores — shared post/wait operations */

int xsem_post(sem_t *sem, int line, char *file) {
    int e = sem_post(sem);
    if (e != 0) {
        perror("sem_post failed");
        fprintf(stderr, "== %d == Line: %d, File: %s\n", getpid(), line, file);
        exit(1);
    }
    return e;
}

int xsem_wait(sem_t *sem, int line, char *file) {
    int e = sem_wait(sem);
    if (e != 0) {
        perror("sem_wait failed");
        fprintf(stderr, "== %d == Line: %d, File: %s\n", getpid(), line, file);
        exit(1);
    }
    return e;
}

/* --------------------------------------------------------------------------
 * POSIX threads
 *
 * Thread functions do not set errno on error; they return the error code
 * directly.  xperror() handles printing for these cases.
 * -------------------------------------------------------------------------- */

#define PERROR_BUF 100

/* Print the error message associated with error code `en`, similar to perror. */
void xperror(int en, char *msg) {
    char buf[PERROR_BUF];
    char *errmsg = strerror_r(en, buf, PERROR_BUF);
    if (msg != NULL)
        fprintf(stderr, "%s: %s\n", msg, errmsg);
    else
        fprintf(stderr, "%s\n", errmsg);
}

int xpthread_create(pthread_t *thread, const pthread_attr_t *attr,
                    void *(*start_routine)(void *), void *arg,
                    int line, char *file) {
    int e = pthread_create(thread, attr, start_routine, arg);
    if (e != 0) {
        xperror(e, "pthread_create failed");
        fprintf(stderr, "== %d == Line: %d, File: %s\n", getpid(), line, file);
        pthread_exit(NULL);
    }
    return e;
}

int xpthread_join(pthread_t thread, void **retval, int line, char *file) {
    int e = pthread_join(thread, retval);
    if (e != 0) {
        xperror(e, "pthread_join failed");
        fprintf(stderr, "== %d == Line: %d, File: %s\n", getpid(), line, file);
        pthread_exit(NULL);
    }
    return e;
}

/* --------------------------------------------------------------------------
 * Mutexes
 * -------------------------------------------------------------------------- */

int xpthread_mutex_init(pthread_mutex_t *restrict mutex,
                        const pthread_mutexattr_t *restrict attr,
                        int line, char *file) {
    int e = pthread_mutex_init(mutex, attr);
    if (e != 0) {
        xperror(e, "pthread_mutex_init failed");
        fprintf(stderr, "== %d == Line: %d, File: %s\n", getpid(), line, file);
        pthread_exit(NULL);
    }
    return e;
}

int xpthread_mutex_destroy(pthread_mutex_t *mutex, int line, char *file) {
    int e = pthread_mutex_destroy(mutex);
    if (e != 0) {
        xperror(e, "pthread_mutex_destroy failed");
        fprintf(stderr, "== %d == Line: %d, File: %s\n", getpid(), line, file);
        pthread_exit(NULL);
    }
    return e;
}

int xpthread_mutex_lock(pthread_mutex_t *mutex, int line, char *file) {
    int e = pthread_mutex_lock(mutex);
    if (e != 0) {
        xperror(e, "pthread_mutex_lock failed");
        fprintf(stderr, "== %d == Line: %d, File: %s\n", getpid(), line, file);
        pthread_exit(NULL);
    }
    return e;
}

int xpthread_mutex_unlock(pthread_mutex_t *mutex, int line, char *file) {
    int e = pthread_mutex_unlock(mutex);
    if (e != 0) {
        xperror(e, "pthread_mutex_unlock failed");
        fprintf(stderr, "== %d == Line: %d, File: %s\n", getpid(), line, file);
        pthread_exit(NULL);
    }
    return e;
}

/* --------------------------------------------------------------------------
 * Condition variables
 * -------------------------------------------------------------------------- */

int xpthread_cond_init(pthread_cond_t *restrict cond,
                       const pthread_condattr_t *restrict attr,
                       int line, char *file) {
    int e = pthread_cond_init(cond, attr);
    if (e != 0) {
        xperror(e, "pthread_cond_init failed");
        fprintf(stderr, "== %d == Line: %d, File: %s\n", getpid(), line, file);
        pthread_exit(NULL);
    }
    return e;
}

int xpthread_cond_destroy(pthread_cond_t *cond, int line, char *file) {
    int e = pthread_cond_destroy(cond);
    if (e != 0) {
        xperror(e, "pthread_cond_destroy failed");
        fprintf(stderr, "== %d == Line: %d, File: %s\n", getpid(), line, file);
        pthread_exit(NULL);
    }
    return e;
}

int xpthread_cond_wait(pthread_cond_t *restrict cond,
                       pthread_mutex_t *restrict mutex,
                       int line, char *file) {
    int e = pthread_cond_wait(cond, mutex);
    if (e != 0) {
        xperror(e, "pthread_cond_wait failed");
        fprintf(stderr, "== %d == Line: %d, File: %s\n", getpid(), line, file);
        pthread_exit(NULL);
    }
    return e;
}

int xpthread_cond_signal(pthread_cond_t *cond, int line, char *file) {
    int e = pthread_cond_signal(cond);
    if (e != 0) {
        xperror(e, "pthread_cond_signal failed");
        fprintf(stderr, "== %d == Line: %d, File: %s\n", getpid(), line, file);
        pthread_exit(NULL);
    }
    return e;
}

int xpthread_cond_broadcast(pthread_cond_t *cond, int line, char *file) {
    int e = pthread_cond_broadcast(cond);
    if (e != 0) {
        xperror(e, "pthread_cond_broadcast failed");
        fprintf(stderr, "== %d == Line: %d, File: %s\n", getpid(), line, file);
        pthread_exit(NULL);
    }
    return e;
}
