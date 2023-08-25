#include <stdio.h>
#include <stdarg.h>

#define LOG2FILE_DB_SIZE        (16)

static struct log2file_inter {
    char name[1024];
    FILE* fp;
    int enable;
} log2file_db[LOG2FILE_DB_SIZE] = { 0 };

static int log2file_db_cnt = 0;

static char log2file_buffer[1024];

void log2file(const char* filename, char* fmt, ...) {
    va_list args;
    int i;

    for (i = 0; i < log2file_db_cnt; i++) {
        if (0 == strcmp(filename, log2file_db[i].name)) {
            break;
        }
    }

    // Create a new entry
    if (i == log2file_db_cnt) {
        if (log2file_db_cnt < LOG2FILE_DB_SIZE) {
            strncpy(log2file_db[i].name, filename, 1024);
            log2file_db[i].fp = fopen(filename, "w+");

            if (log2file_db[i].fp) {
                log2file_db_cnt++;
            }
        }
    }

    if (NULL != log2file_db[i].fp) {
        va_start(args, fmt);
        vsnprintf(log2file_buffer, 1024, fmt, args);
        va_end(args);

        fprintf(log2file_db[i].fp, "%s", log2file_buffer);
        fflush(log2file_db[i].fp);
    }
}