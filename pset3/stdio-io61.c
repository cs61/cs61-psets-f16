#include "io61.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <errno.h>

// stdio-io61.c
//    This version of io61.c is a simple wrapper on stdio. Can you beat it?


struct io61_file {
    FILE* f;
};


io61_file* io61_fdopen(int fd, int mode) {
    assert(fd >= 0);
    io61_file* f = (io61_file*) malloc(sizeof(io61_file));
    f->f = fdopen(fd, mode == O_RDONLY ? "r" : "w");
    return f;
}

int io61_close(io61_file* f) {
    io61_flush(f);
    int r = fclose(f->f);
    free(f);
    return r;
}


int io61_readc(io61_file* f) {
    return fgetc(f->f);
}

ssize_t io61_read(io61_file* f, char* buf, size_t sz) {
    size_t n = fread(buf, 1, sz, f->f);
    if (n != 0 || sz == 0 || !ferror(f->f))
        return (ssize_t) n;
    else
        return (ssize_t) -1;
}


int io61_writec(io61_file* f, int ch) {
    return fputc(ch, f->f);
}

ssize_t io61_write(io61_file* f, const char* buf, size_t sz) {
    size_t n = fwrite(buf, 1, sz, f->f);
    if (n != 0 || sz == 0 || !ferror(f->f))
        return (ssize_t) n;
    else
        return (ssize_t) -1;
}


int io61_flush(io61_file* f) {
    return fflush(f->f);
}

int io61_seek(io61_file* f, off_t pos) {
    return fseek(f->f, pos, SEEK_SET);
}


io61_file* io61_open_check(const char* filename, int mode) {
    int fd;
    if (filename)
        fd = open(filename, mode, 0666);
    else if ((mode & O_ACCMODE) == O_RDONLY)
        fd = STDIN_FILENO;
    else
        fd = STDOUT_FILENO;
    if (fd < 0) {
        fprintf(stderr, "%s: %s\n", filename, strerror(errno));
        exit(1);
    }
    return io61_fdopen(fd, mode & O_ACCMODE);
}

off_t io61_filesize(io61_file* f) {
    struct stat s;
    int r = fstat(fileno(f->f), &s);
    if (r >= 0 && S_ISREG(s.st_mode))
        return s.st_size;
    else
        return -1;
}

int io61_eof(io61_file* f) {
    return feof(f->f);
}
