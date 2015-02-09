/**
 * User-space filesystem that works with empty files.
 */

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/queue.h>

static struct elimits
{
    size_t max;
    size_t cur;
};
static struct eoptions
{
    struct elimits write;
} options = {
    { (size_t)-1 },
};

LIST_HEAD(efiles_head_t, efile) efiles_head;
struct efile
{
    char name[PATH_MAX];
    size_t size;
    LIST_ENTRY(efile) entries;
};
static struct efile *efiles;
struct efile *efiles_add(const char *name)
{
    struct efile *f = malloc(sizeof(struct efile));
    if (f) {
        strncpy(f->name, name, PATH_MAX - 1);
        f->size = 0;
        LIST_INSERT_HEAD(&efiles_head, f, entries);
    }
    return f;
}
void efiles_del(const char *name)
{
    struct efile *f, *f2;
    for (f = efiles_head.lh_first, f2 = f ? f->entries.le_next : NULL;
         f != NULL;
         f = f2) {
        if (!strcmp(f->name, name)) {
            LIST_REMOVE(efiles_head.lh_first, entries);
        }
    }
}
void efiles_free()
{
    while (efiles_head.lh_first != NULL)  {
        LIST_REMOVE(efiles_head.lh_first, entries);
    }
}
struct efile *efiles_get(const char *name)
{
    struct efile *f;
    for (f = efiles_head.lh_first; f != NULL; f = f->entries.le_next) {
        if (!strcmp(f->name, name)) {
            break;
        }
    }
    return f;
}

static int empty_getattr(const char *path, struct stat *stbuf)
{
    int res = 0;
    struct efile *f;

    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    } else if ((f = efiles_get(path))) {
        stbuf->st_mode = S_IFREG | 0666;
        stbuf->st_nlink = 1;
        stbuf->st_size = f->size;
    } else {
        res = -ENOENT;
    }

    return res;
}
static int empty_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                         off_t offset, struct fuse_file_info *fi)
{
    (void)offset;
    (void)fi;
    struct efile *f;

    if (strcmp(path, "/") != 0) {
        return -ENOENT;
    }

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    for (f = efiles_head.lh_first; f != NULL; f = f->entries.le_next) {
        filler(buf, f->name + 1, NULL, 0);
    }

    return 0;
}
static int empty_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    (void)fi;
    (void)mode;

    if (!efiles_get(path) && !efiles_add(path)) {
        return -ENOENT;
    }
    return 0;
}
static int empty_utimens(const char *path, const struct timespec tv[2])
{
    (void)tv;
    (void)path;
    return 0;
}
static int empty_open(const char *path, struct fuse_file_info *fi)
{
    (void)fi;

    if (!efiles_get(path)) {
        return -ENOENT;
    }
    return 0;
}
static int empty_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
    (void)fi;

    struct efile *f = efiles_get(path);
    if (!f) {
        return -ENOENT;
    }

    if (offset < f->size) {
        if ((offset + size) > f->size) {
            size = f->size - offset;
        }
        memset(buf, 0, size);
    } else {
        size = 0;
    }

    return size;
}
static int empty_write(const char *path, const char *buf, size_t size, off_t offset,
                       struct fuse_file_info *fi)
{
    (void)fi;

    struct efile *f = efiles_get(path);
    if (!f) {
        return -ENOENT;
    }

    size_t left = options.write.max - options.write.cur;
    if (!left) {
        return -ENOSPC;
    } else if (size > left) {
        size = left;
    }
    options.write.cur += size;

    if (!offset) {
        f->size = size;
    } else if ((offset + size) > f->size) {
        f->size -= offset;
        f->size += size;
    }
    return size;
}

static struct fuse_operations empty_ops = {
    .getattr    = empty_getattr,
    .readdir    = empty_readdir,
    .create     = empty_create,
    .utimens    = empty_utimens,
    .open       = empty_open,
    .read       = empty_read,
    .write      = empty_write,
};

void parse_options()
{
    char *max = getenv("EMPTY_FUSEFS_MAX");
    if (max) {
        options.write.max = atoll(max);
    }
}
int main(int argc, char **argv)
{
    int ret;
    parse_options();
    LIST_INIT(&efiles_head);
    ret = fuse_main(argc, argv, &empty_ops, NULL);
    efiles_free();
    return ret;
}
