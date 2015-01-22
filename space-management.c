#define _GNU_SOURCE
#include <stdlib.h>
#include <dlfcn.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <assert.h>
#ifdef SM_MONITOR
#include <sys/statfs.h>
#endif


struct Options
{
    size_t retries;
    size_t retried;

    int verbose;
    int permanent;

#ifdef SM_MONITOR
    struct {
        size_t interval;
        size_t max;
        /** TODO: support multiple mount points */
        const char *mnt;
    } m;
#endif
} options = {
    .retries = (size_t)-1,
    .m = {
        .interval = 10,
        .max = 90,
        .mnt = "/",
    },
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

#ifdef SM_MONITOR
pthread_t monitor;
void *monitor_cb(void *arg)
{
    struct statfs fs;
    (void)arg;

    for (;;) {
        if (statfs(options.m.mnt, &fs)) {
            vePrintf("Failed to stat '%s'\n", options.m.mnt);
        }
        /** XXX: f_bsize */
        size_t free = fs.f_bfree * fs.f_bsize;
        size_t total = (fs.f_bfree + fs.f_bavail) * fs.f_bsize;
        options.permanent = (free < (total * options.m.max));
        sleep(options.m.interval);
    }
}
#endif

__attribute__((constructor)) static void write_init(void)
{
    write_orig = dlsym(RTLD_NEXT, "write");
    vPrintf("initializing\n");

    if (getenv("SPACE_MANAGEMENT_VERBOSE")) {
        ++options.verbose;
        vPrintf("verbose: on\n");
    }

    char *retries = getenv("SPACE_MANAGEMENT_RETRIES");
    if (retries) {
        options.retries = atoll(retries);
        vPrintf("retries: %zu\n", options.retries);
    }

#ifdef SM_MONITOR
    char *monitor = getenv("SPACE_MANAGEMENT_MONITOR");
    if (monitor) {
        char *interval = getenv("SPACE_MANAGEMENT_MONITOR_INTERVAL");
        char *max = getenv("SPACE_MANAGEMENT_MONITOR_MAX");
        char *mnt = getenv("SPACE_MANAGEMENT_MONITOR_MNT");
        if (interval) {
            options.m.interval = atoll(interval);
            vPrintf("monitor interval: %zu\n", options.m.interval);
        }
        if (max) {
            options.m.max = atoll(max);
            vPrintf("monitor max: %zu\n", options.m.max);
        }
        if (mnt) {
            options.m.mnt = mnt;
            vPrintf("monitor mnt: %s\n", options.m.mnt);
        }

        assert(!pthread_create(&monitor, NULL, monitor_cb, NULL));
    }
#endif
}
#ifdef SM_MONITOR
__attribute__((destructor)) static void write_deinit(void)
{
    assert(!pthread_cancel(monitor, NULL));
}
#endif


static void stop_process()
{
    pid_t pid = getpid();
    vePrintf("stopping %i because of ENOSPC\n", pid);
    kill(pid, SIGSTOP);
}

ssize_t write(int fd, const void *buf, size_t count)
{
    ssize_t ret;

    for (;;) {
        if (options.permanent) {
            stop_process();
            continue;
        }

        ret = write_orig(fd, buf, count);
        if (ret == -1 && errno == ENOSPC &&
            options.retried++ < options.retries) {
            stop_process();
            continue;
        }
        options.retried = 0;
        break;
    }
    return ret;
}
