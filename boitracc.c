#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <fcntl.h>
#include <poll.h>

#define NS_PER_S 1000000000L
#define NS_PER_MS 1000000L

const char* boi_path = "/home/e/.local/share/binding of isaac afterbirth+/log.txt";
const struct timespec ts_offs = {1, 100*NS_PER_MS};

struct timespec ts_sub(struct timespec ts1, struct timespec ts2)
{
    struct timespec result;
    result.tv_sec = ts1.tv_sec - ts2.tv_sec;
    if (ts1.tv_nsec < ts2.tv_nsec) {
        result.tv_nsec = ts1.tv_nsec + NS_PER_S - ts2.tv_nsec;
        result.tv_sec--;
    } else {
        result.tv_nsec = ts1.tv_nsec - ts2.tv_nsec;
    }
    return (result);
}

struct timespec ts_add(struct timespec ts1, struct timespec ts2)
{
    struct timespec result;
    result.tv_sec = ts1.tv_sec + ts2.tv_sec;
    result.tv_nsec = ts1.tv_nsec + ts2.tv_nsec;
    if (result.tv_nsec >= NS_PER_S) {
        result.tv_sec++;
        result.tv_nsec = result.tv_nsec - NS_PER_S;
    }
    return (result);
}

int main()
{
    int boi_fd = open(boi_path, O_RDONLY | O_NONBLOCK);
    //fcntl(boi_fd, F_SETFL, fcntl(boi_fd, F_GETFL) | O_NONBLOCK);
    FILE* boi = fdopen(boi_fd, "r");

    struct timespec ts_start, ts_now, ts_time;
    bool timer_running = false;

    char* line_s = NULL;
    size_t line_n = 0;

    char* seed_s = NULL;
    long seed_i;

    char* end_s = NULL;
    int end_i;

    while (true) {
        if (poll(&(struct pollfd){boi_fd, POLLIN, 0}, 1, 16)) {
            free(line_s);
            line_s = NULL;
            line_n = 0;
            if (!getline(&line_s, &line_n, boi)) {
                fprintf(stderr, "ERROR: getline failed\n");
                return 1;
            }
            //printf("\x1b[34m%s\x1b[0m", line_s);

            if (sscanf(line_s, "[INFO] - RNG Start Seed: %m[ 0-9A-Z] (%ld)", &seed_s, &seed_i)) {
                printf("\n NEW RUN    seed: %s\n", seed_s);
                clock_gettime(CLOCK_REALTIME, &ts_start);
                ts_start = ts_add(ts_start, ts_offs);
                timer_running = true;
                free(seed_s);
            }
            else if (sscanf(line_s, "[INFO] - playing cutscene %d (%m[ a-zA-Z]).", &end_i, &end_s)) {
                printf("time! %s ending\n", end_s);
                timer_running = false;
                free(end_s);
            }
        }

        if (timer_running) {
            clock_gettime(CLOCK_REALTIME, &ts_now);
            ts_time = ts_sub(ts_now, ts_start);
            int h = (int)ts_time.tv_sec/3600;
            int m = (int)ts_time.tv_sec/60 - h*60;
            int s = (int)ts_time.tv_sec - h*360 - m*60;
            int ms = (int)ts_time.tv_nsec/NS_PER_MS;
            if (s < 0 || ms < 0) { s = 0; ms = 0; }
            if (h == 0 && m == 0) printf("\r     %2d.%03d \t", s, ms);
            else if (h == 0) printf("\r  %2d:%02d.%03d \t", m, s, ms);
            else printf("\r%d:%02d:%02d.%03d \t", h, m, s, ms);
        }

        fflush(stdout);
    }
    return 0;
}
