/* LRU testcase:

*/
#define SIZE 130
#define ITER 3000

int a[SIZE];

int main() {

	for (a[2] = 1; a[2] < ITER; a[2]++) {
		a[1] = a[0];
		a[1] = a[32];
		a[1] = a[64];
		a[1] = a[96];
		a[1] = a[128];
	}

}