#define _GNU_SOURCE
#include <stdlib.h>
#include <dlfcn.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

static int initialized = 0;
static ssize_t (*write_orig)(int fd, const void *buf, size_t count) = NULL;

static void write_init()
{
    write_orig = dlsym(RTLD_NEXT, "write");
}

ssize_t write(int fd, const void *buf, size_t count)
{
    ssize_t ret;

    if (!initialized) {
        write_init();
    }

    for (;;) {
        ret = write_orig(fd, buf, count);
        if (ret == -1 && errno == ENOSPC) {
            kill(getpid(), SIGSTOP);
            continue;
        }
        break;
    }
    return ret;
}
