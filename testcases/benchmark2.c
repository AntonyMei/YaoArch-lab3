/**
 * Author:    Yiwei Li (liyw19@mails.tsinghua.edu.cn)
 * Created:   2021/05/09
 * 
 * Yao-archlab
 **/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// scheduler-related variables
// modify SCHED_INTERVAL to show different cache access patterns
// (lower value leads to more chaos)
#define SCHED_INTERVAL 50
#define THREAD_MAX 5
int sched_op_cnt = 0;
int thread_num = 0;
int sched_finish = 0;
uint32_t sched_cur_thread = -1U;
typedef enum thread_state {
    // THREAD_INIT,
    THREAD_READY,
    // THREAD_RUNNING, // no need for these states so far
    THREAD_EXIT,
} thread_state;

typedef struct sched_thread {
    void (*func)(void);
    uint32_t arg1, arg2, arg3, arg4; // states that need to be saved/recovered.
                                     // Add more if you want
    thread_state state;
} sched_thread;
sched_thread thread_arr[THREAD_MAX];

#define MAT_VALUE_MAX 10
#define MAT_SHAPE_STEP 1
// int mat_shape_list[] = {2, 7, 32, 100, 128, 512};
int mat_shape_list[] = {32, 32, 32};
// 10*10*10 gemm should be simulated within 10s on Ripes single-cycle processor model
#define MAT_SHAPE_LEN (sizeof(mat_shape_list) / sizeof(int))
int mat_shape_idx = 0;

void sched_add_thread(void (*func)) {
    thread_arr[thread_num].arg1 = 0;
    thread_arr[thread_num].arg2 = 0;
    thread_arr[thread_num].arg3 = 0;
    thread_arr[thread_num].arg4 = 0;
    thread_arr[thread_num].func = func;
    thread_arr[thread_num].state = THREAD_READY;
    thread_num += 1;
}

void sched_save_context(uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4) {
    thread_arr[sched_cur_thread].arg1 = arg1;
    thread_arr[sched_cur_thread].arg2 = arg2;
    thread_arr[sched_cur_thread].arg3 = arg3;
    thread_arr[sched_cur_thread].arg4 = arg4;
    sched_op_cnt = 0;
}

void sched_load_context(uint32_t* arg1, uint32_t* arg2, uint32_t* arg3, uint32_t* arg4) {
    *arg1 = thread_arr[sched_cur_thread].arg1;
    *arg2 = thread_arr[sched_cur_thread].arg2;
    *arg3 = thread_arr[sched_cur_thread].arg3;
    *arg4 = thread_arr[sched_cur_thread].arg4;
}

void sched_exit() {
    thread_arr[sched_cur_thread].state = THREAD_EXIT;
    int all_done = 1;
    for (uint32_t i = 0; i < thread_num; i++) {
        if (thread_arr[i].state != THREAD_EXIT) {
            all_done = 0;
        }
    }
    if (all_done == 1) sched_finish = 1;
}

void mat_display(uint32_t *mat, uint32_t n, uint32_t m) {
    for (uint32_t i = 0; i < n; i++) {
        for (uint32_t j = 0; j < m; j++) {
            printf("%d ", mat[i * m + j]);
        }
        printf("\n");
    }
    printf("\n");
}

void mat_mul1() {
    static int needInit = 1;
    uint32_t ci_start = 0, cj_start = 0, ck_start = 0, cijk_tmp = 0;
    static uint32_t a_n, a_m, b_n, b_m, c_n, c_m;
    static uint32_t *A, *B, *C;
    if (needInit > 0) {
        a_n = mat_shape_list[mat_shape_idx];
        mat_shape_idx = (mat_shape_idx + MAT_SHAPE_STEP) % MAT_SHAPE_LEN;
        a_m = mat_shape_list[mat_shape_idx];
        mat_shape_idx = (mat_shape_idx + MAT_SHAPE_STEP) % MAT_SHAPE_LEN;
        b_m = mat_shape_list[mat_shape_idx];
        // printf("%d %d %d %d\n", a_n, a_m, b_m, MAT_SHAPE_LEN);
        b_n = a_m, c_n = a_n, c_m = b_m;
        A = (uint32_t *)malloc(sizeof(uint32_t) * a_n * a_m);
        B = (uint32_t *)malloc(sizeof(uint32_t) * b_n * b_m);
        C = (uint32_t *)malloc(sizeof(uint32_t) * c_n * c_m);
        // intialization
        for (uint32_t i = 0; i < a_n; i++) {
            for (uint32_t j = 0; j < a_m; j++) {
                A[i * a_m + j] = rand() % MAT_VALUE_MAX;
            }
        }
        for (uint32_t i = 0; i < b_n; i++) {
            for (uint32_t j = 0; j < b_m; j++) {
                B[i * b_m + j] = rand() % MAT_VALUE_MAX;
            }
        }
        // mat_display(A, a_n, a_m);
        // mat_display(B, b_n, b_m);
        needInit = 0;
    } else {
        uint32_t dummy;
        sched_load_context(&ci_start, &cj_start, &ck_start, &cijk_tmp);
        // printf("recover from %d %d %d\n", ci_start, cj_start, ck_start);
    }
    uint32_t i = ci_start, j = cj_start, k = ck_start;
    C[i * c_m + j] = cijk_tmp;
    while (i < c_n) {
        C[i * c_m + j] += A[i * a_m + k] * B[k * b_m + j];
        // printf("%d %d %d\n", i, j, k);
        k++;
        if (k >= a_m) {
            j++;
            k = 0;
            if (j >= c_m) {
                i++;
                j = 0;
            }
            C[i * c_m + j] = 0;
        }
        if (sched_op_cnt++ >= SCHED_INTERVAL - 1) {
            sched_save_context(i, j, k, C[i * c_m + j]);
            // printf("save %d %d %d\n", i, j, k);
            return; // yield
        }
    }
    // mat_display(C, c_n, c_m);
    free(A);
    free(B);
    free(C);
    sched_exit();
}

void mat_mul2() {
    static int needInit = 1;
    uint32_t ci_start = 0, cj_start = 0, ck_start = 0, cijk_tmp = 0;
    static uint32_t a_n, a_m, b_n, b_m, c_n, c_m;
    static uint32_t *A, *B, *C;
    if (needInit > 0) {
        a_n = mat_shape_list[mat_shape_idx];
        mat_shape_idx = (mat_shape_idx + MAT_SHAPE_STEP) % MAT_SHAPE_LEN;
        a_m = mat_shape_list[mat_shape_idx];
        mat_shape_idx = (mat_shape_idx + MAT_SHAPE_STEP) % MAT_SHAPE_LEN;
        b_m = mat_shape_list[mat_shape_idx];
        // printf("%d %d %d %d\n", a_n, a_m, b_m, MAT_SHAPE_LEN);
        b_n = a_m, c_n = a_n, c_m = b_m;
        A = (uint32_t *)malloc(sizeof(uint32_t) * a_n * a_m);
        B = (uint32_t *)malloc(sizeof(uint32_t) * b_n * b_m);
        C = (uint32_t *)malloc(sizeof(uint32_t) * c_n * c_m);
        // intialization
        for (uint32_t i = 0; i < a_n; i++) {
            for (uint32_t j = 0; j < a_m; j++) {
                A[i * a_m + j] = rand() % MAT_VALUE_MAX;
            }
        }
        for (uint32_t i = 0; i < b_n; i++) {
            for (uint32_t j = 0; j < b_m; j++) {
                B[i * b_m + j] = rand() % MAT_VALUE_MAX;
            }
        }
        // mat_display(A, a_n, a_m);
        // mat_display(B, b_n, b_m);
        needInit = 0;
    } else {
        sched_load_context(&ci_start, &cj_start, &ck_start, &cijk_tmp);
        // printf("recover from %d %d %d\n", ci_start, cj_start, ck_start);
    }
    uint32_t i = ci_start, j = cj_start, k = ck_start;
    C[i * c_m + j] = cijk_tmp;
    while (i < c_n) {
        C[i * c_m + j] += A[i * a_m + k] * B[k * b_m + j];
        // printf("%d %d %d\n", i, j, k);
        k++;
        if (k >= a_m) {
            j++;
            k = 0;
            if (j >= c_m) {
                i++;
                j = 0;
            }
            C[i * c_m + j] = 0;
        }
        if (sched_op_cnt++ >= SCHED_INTERVAL - 1) {
            sched_save_context(i, j, k, C[i * c_m + j]);
            // printf("save %d %d %d\n", i, j, k);
            return; // yield
        }
    }
    // mat_display(C, c_n, c_m);
    free(A);
    free(B);
    free(C);
    sched_exit();
}

int main() {
    sched_add_thread(mat_mul1);
    sched_add_thread(mat_mul2);
    // add more tasks if you like
    uint32_t next_thread = -1U;
    while (sched_finish == 0) {
        next_thread = rand() % thread_num;
        if (thread_arr[next_thread].state == THREAD_READY) {
            sched_cur_thread = next_thread;
            // printf("switch to next thread %d\n", sched_cur_thread);
            thread_arr[next_thread].func();
        }
    }
    return 0;
}