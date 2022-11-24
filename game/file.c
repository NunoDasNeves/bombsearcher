#include<stdio.h>
#include<SDL.h>
#include"types.h"
#include"log.h"
#include"mem.h"
#include"file.h"

char *file_read(const char *filename, u64 *len, bool to_string)
{
    ASSERT(filename);

    i64 extra_char = to_string ? 1 : 0;

    if (*len > (u64)(INT64_MAX - extra_char)) { // leave 1 space for null char
        log_error("len too large: %" PRIu64 "", *len);
        return NULL;
    }

    char* buffer = NULL;
    i64 buf_size;
    i64 size, objs_read;
    SDL_RWops *file = SDL_RWFromFile(filename, "rb");

    if (file == NULL) {
        log_error("SDL Error: %s\n", SDL_GetError());
        return NULL;
    }

    if (*len == 0) {
        size = SDL_RWsize(file);
        if (size < 0) {
            log_error("SDL Error: %s\n", SDL_GetError());
            goto err_close_file;
        }
    } else {
        size = *len;
    }

    buf_size = size + extra_char;
    buffer = mem_alloc(buf_size);

    if (buffer == NULL) {
        log_error("alloc of read buffer failed\n");
        goto err_close_file;
    }

    objs_read = SDL_RWread(file, buffer, size, 1);
    if (objs_read <= 0) {
        log_error("SDL Error: %s\n", SDL_GetError());
        goto err_free_buffer;
    }
    if (objs_read != 1) {
        log_error("Read less than expected: read %" PRId64 ", expected 1\n", objs_read);
        goto err_free_buffer;
    }

    if (SDL_RWclose(file)) {
        log_error("SDL Error: %s\n", SDL_GetError());
    }

    if (to_string) {
        buffer[size] = '\0';
    }
    *len = size;

    return buffer;

err_free_buffer:
    mem_free(buffer);
err_close_file:
    SDL_RWclose(file);
    return NULL;
}
