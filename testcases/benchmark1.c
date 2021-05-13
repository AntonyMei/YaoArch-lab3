#include "stdio.h"

#define NODE_NUMBER 10
#define EDGE_NUMBER 50
#define MAX_ITER 10

char* read_file = "/home/arch_stu2019011041/HW/lab3/benchmark1_data.txt";

int main() {
    FILE *fp = NULL;
    fp = fopen(read_file, "r");
    if (fp == NULL) {
        printf("File does not exist!\n");
        return 0;
    }
    double equal_prob = 1.0 / NODE_NUMBER;
    int neighbor[NODE_NUMBER][NODE_NUMBER];
    double rank_new[NODE_NUMBER];
    double rank_old[NODE_NUMBER];
    int outgoing_size[NODE_NUMBER];
    int ingoing_size[NODE_NUMBER];
    for (int i = 0; i < NODE_NUMBER; ++i) {
        rank_old[i] = equal_prob;
        ingoing_size[i] = 0;
        outgoing_size[i] = 0;
    }
    int src = 0, dst = 0;
    for (int i = 0; i < EDGE_NUMBER; ++i) {
        fscanf(fp, "%d %d", &src, &dst);
        outgoing_size[src]++;
        neighbor[dst][ingoing_size[dst]++] = src;
    }
    
    double broadcastScore = 0.0;
    for (int iter = 0; iter < MAX_ITER; ++iter) {
        broadcastScore = 0.0;
        for (int i = 0; i < NODE_NUMBER; ++i) {
            rank_new[i] = 0.0;
            if (outgoing_size[i] == 0) {
                broadcastScore += rank_old[i];
            }
            for (int j = 0; j < ingoing_size[i]; ++j) {
                int dst = neighbor[i][j];
                rank_new[i] += rank_old[dst] / outgoing_size[dst];
            }
            rank_new[i] = 0.85 * rank_new[i] + 0.15 * equal_prob;
        }
        for (int i = 0; i < NODE_NUMBER; ++i) {
            rank_new[i] += 0.85 * broadcastScore * equal_prob;
        }
        for (int i = 0; i < NODE_NUMBER; ++i) {
            rank_old[i] = rank_new[i];
        }
    }
    for (int i = 0; i < NODE_NUMBER; ++i) {
        printf("%f\n", rank_old[i]);
    }
    return 0;
}
