/* ============================================================================
 * errcheck.h
 * Checked wrappers around common POSIX system calls.
 *
 * Every wrapper mirrors the signature of its underlying call but terminates
 * (or calls pthread_exit) on failure, printing a diagnostic message that
 * includes the source file and line number via the QUI macro.
 *
 * Usage example:
 *   FILE *f = xfopen("data.txt", "r", QUI);
 * ============================================================================ */

#ifndef ERRCHECK_H
#define ERRCHECK_H

#define _GNU_SOURCE     /* Enable GNU extensions (strerror_r, etc.)          */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>      /* O_* constants                                      */
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>   /* Mode constants                                     */
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* Expands to the current line number and file name, suitable for passing to
 * any x-prefixed function as the trailing (linea, file) arguments.         */
#define QUI __LINE__, __FILE__

/* --------------------------------------------------------------------------
 * Process termination helpers
 * -------------------------------------------------------------------------- */
void termina(const char *msg);
void xtermina(const char *msg, int line, char *file);

/* --------------------------------------------------------------------------
 * FILE * operations
 * -------------------------------------------------------------------------- */
FILE *xfopen(const char *path, const char *mode, int line, char *file);

/* --------------------------------------------------------------------------
 * File descriptor operations
 * -------------------------------------------------------------------------- */
void xclose(int fd, int line, char *file);

/* --------------------------------------------------------------------------
 * Process operations
 * -------------------------------------------------------------------------- */
pid_t xfork(int line, char *file);
pid_t xwait(int *status, int line, char *file);

/* --------------------------------------------------------------------------
 * Pipe operations
 * -------------------------------------------------------------------------- */
int xpipe(int pipefd[2], int line, char *file);

/* --------------------------------------------------------------------------
 * POSIX shared memory
 * -------------------------------------------------------------------------- */
int   xshm_open(const char *name, int oflag, mode_t mode, int line, char *file);
int   xshm_unlink(const char *name, int line, char *file);
int   xftruncate(int fd, off_t length, int line, char *file);
void *simple_mmap(size_t length, int fd, int line, char *file);
int   xmunmap(void *addr, size_t length, int line, char *file);

/* --------------------------------------------------------------------------
 * POSIX semaphores (named and unnamed)
 * -------------------------------------------------------------------------- */
sem_t *xsem_open(const char *name, int oflag, mode_t mode,
                 unsigned int value, int line, char *file);
int    xsem_unlink(const char *name, int line, char *file);
int    xsem_close(sem_t *sem, int line, char *file);
int    xsem_init(sem_t *sem, int pshared, unsigned int value,
                 int line, char *file);
int    xsem_destroy(sem_t *sem, int line, char *file);
int    xsem_post(sem_t *sem, int line, char *file);
int    xsem_wait(sem_t *sem, int line, char *file);

/* --------------------------------------------------------------------------
 * POSIX threads
 * -------------------------------------------------------------------------- */
void xperror(int en, char *msg);

int xpthread_create(pthread_t *thread, const pthread_attr_t *attr,
                    void *(*start_routine)(void *), void *arg,
                    int line, char *file);
int xpthread_join(pthread_t thread, void **retval, int line, char *file);

/* --------------------------------------------------------------------------
 * Mutexes
 * -------------------------------------------------------------------------- */
int xpthread_mutex_init(pthread_mutex_t *restrict mutex,
                        const pthread_mutexattr_t *restrict attr,
                        int line, char *file);
int xpthread_mutex_destroy(pthread_mutex_t *mutex, int line, char *file);
int xpthread_mutex_lock(pthread_mutex_t *mutex, int line, char *file);
int xpthread_mutex_unlock(pthread_mutex_t *mutex, int line, char *file);

/* --------------------------------------------------------------------------
 * Condition variables
 * -------------------------------------------------------------------------- */
int xpthread_cond_init(pthread_cond_t *restrict cond,
                       const pthread_condattr_t *restrict attr,
                       int line, char *file);
int xpthread_cond_destroy(pthread_cond_t *cond, int line, char *file);
int xpthread_cond_wait(pthread_cond_t *restrict cond,
                       pthread_mutex_t *restrict mutex,
                       int line, char *file);
int xpthread_cond_signal(pthread_cond_t *cond, int line, char *file);
int xpthread_cond_broadcast(pthread_cond_t *cond, int line, char *file);

#endif /* ERRCHECK_H */
