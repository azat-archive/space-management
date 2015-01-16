#define _GNU_SOURCE
#include <stdlib.h>
#include <dlfcn.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>

struct Options
{
    size_t retries;
    size_t retried;

    int initialized;
    int verbose;
} options = {
    .retries = (size_t)-1,
    .verbose = 0,
    .initialized = 0,
};

#define vPrintf(...) do {                               \
    if (options.verbose >= 1) {                         \
        printf("space-management: " __VA_ARGS__);       \
    }                                                   \
} while (0);
#define vePrintf(...) do {                                  \
    if (options.verbose >= 1) {                             \
        fprintf(stderr, "space-management: " __VA_ARGS__);  \
    }                                                       \
} while (0);


static ssize_t (*write_orig)(int fd, const void *buf, size_t count) = NULL;

static void write_init()
{
    write_orig = dlsym(RTLD_NEXT, "write");
    vPrintf("initialized\n");
    options.initialized = 1;

    if (getenv("SPACE_MANAGEMENT_VERBOSE")) {
        ++options.verbose;
        vPrintf("verbose: on\n");
    }

    char *retries = getenv("SPACE_MANAGEMENT_RETRIES");
    if (retries) {
        options.retries = atoll(retries);
        vPrintf("retries: %zu\n", options.retries);
    }
}

ssize_t write(int fd, const void *buf, size_t count)
{
    ssize_t ret;

    if (!options.initialized) {
        write_init();
    }

    for (;;) {
        ret = write_orig(fd, buf, count);
        if (ret == -1 && errno == ENOSPC &&
            options.retried++ < options.retries) {
            pid_t pid = getpid();
            vePrintf("stopping %i because of ENOSPC\n", pid);
            kill(pid, SIGSTOP);
            continue;
        }
        options.retried = 0;
        break;
    }
    return ret;
}
