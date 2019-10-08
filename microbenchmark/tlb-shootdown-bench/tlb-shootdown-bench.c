#include <stdio.h>
#include <stdlib.h>
#define __USE_GNU
#include <sched.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/mman.h>

pid_t __gettid();
suseconds_t __time_diff();

#define print_err_and_exit(en, msg) \
    do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

#define TRACE() \
    do { printf("[TID %d]TRACE : %s\n", __gettid(), __func__); } while(0);


unsigned long buf_size = 256*1024*1024; //64M
int bench_loops = 1;
int use_unmap = 0;
int use_interleave = 0;


struct thread_info {
    pthread_t thread_id;
    int core_id;
    pid_t tid;
    suseconds_t usec_fill;
    suseconds_t usec_leak;
    int ret;
};


inline pid_t __gettid()
{
    return syscall(__NR_gettid);
}

inline suseconds_t __time_diff(struct timeval *start, struct timeval *end)
{
    return (end->tv_sec - start->tv_sec) * 1000 * 1000 + end->tv_usec - start->tv_usec;
}

void *__routine(void *arg)
{
    struct thread_info *thread_info = arg;
    const pthread_t pid = pthread_self();
    const int core_id = thread_info->core_id;
    struct timeval start, end;
    int ret;

    TRACE();
    thread_info->tid = __gettid();

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    ret = pthread_setaffinity_np(pid, sizeof(cpu_set_t), &cpuset);
    if (ret != 0) {
        print_err_and_exit(ret, "pthread_setaffinity_np");
    }

    ret = pthread_getaffinity_np(pid, sizeof(cpu_set_t), &cpuset);
    if (ret != 0) {
        print_err_and_exit(ret, "pthread_getaffinity_np");
    }

    int index;
    char *p = (char*)mmap(NULL, buf_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    int loop;
    for (loop = 0; loop < bench_loops; loop++) {
        gettimeofday(&start, NULL);
        for (index = 0; index < buf_size; index += 4096)
            *(p+index) = 0;

        gettimeofday(&end, NULL);
        thread_info->usec_fill += __time_diff(&start, &end);

        gettimeofday(&start, NULL);
        if (use_unmap) {
            ret = munmap(p, buf_size);
            if (ret != 0) {
                print_err_and_exit(ret, "munmap");
            }
            //printf("munmap ret = %d\n", ret);
            char *p = (char*)mmap(NULL, buf_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        } else {
            for (index = 0; index < buf_size; index += 4096) {
                ret = madvise(p+index, 4096, MADV_DONTNEED);
                if (ret) {
                    perror("madvise");
                    thread_info->ret = ret;
                    break;
                }
            }
        }
        gettimeofday(&end, NULL);
        thread_info->usec_leak += __time_diff(&start, &end);
    }


    return NULL;
}

void show_help()
{
    printf("Usage :\n");
    printf("\t-l NUM : bench loops\n");
    printf("\t-n CPUs : bench cpus\n");
    printf("\t-u : use munmap instead of madvise\n");
    printf("\t-i : use interleave cpu sequence\n");
}

int main(int argc, char *argv[])
{
    int opt;
    int num_threads = 8;
    suseconds_t fill_elapled = 0, leak_elapled;

    while ((opt = getopt(argc, argv, "il:n:u")) != -1) {
        switch (opt) {
            case 'i':
                use_interleave = 1;
                break;

            case 'l':
                bench_loops = atoi(optarg);
                break;

            case 'n':
                num_threads = atoi(optarg);
                break;

            case 'u':
                use_unmap = 1;
                break;
        }
    }

    printf("Test %d threads\n", num_threads);
    if (use_unmap)
        printf("\tuse_unamp flag = %d\n", use_unmap);
    if (use_interleave)
        printf("\tuse_interleave flag = %d\n", use_interleave);

    struct thread_info *thread_info = calloc(num_threads, sizeof(struct thread_info));
    if (thread_info == NULL) {
        perror("calloc");
        return 0;
    }

    int tnum;
    for (tnum = 0; tnum < num_threads; tnum++) {
        if (use_interleave)
            thread_info[tnum].core_id = tnum * 2;
        else
            thread_info[tnum].core_id = tnum;
        const int create_result = pthread_create(&thread_info[tnum].thread_id, NULL, __routine, &thread_info[tnum]);
        if (create_result != 0) {
            print_err_and_exit(create_result, "pthread_create");
        }
    }

    fill_elapled = 0;
    leak_elapled = 0;
    for (tnum = 0; tnum < num_threads; tnum++) {
        void *res;
        const int join_result = pthread_join(thread_info[tnum].thread_id, &res);
        if (join_result != 0) {
            print_err_and_exit(join_result, "pthread_join");
        }

        printf("Joined with thread %d; tid %d, ret = %d\n", thread_info[tnum].core_id, thread_info[tnum].tid, thread_info[tnum].ret);
        fill_elapled += thread_info[tnum].usec_fill;
        leak_elapled += thread_info[tnum].usec_leak;
        free(res);
    }

    printf("fill_elapled %ld usec, leak_elapled %ld usec\n", fill_elapled, leak_elapled);

    free(thread_info);
    return 0;
}

