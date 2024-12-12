#include <stdio.h>
#include <cuda_runtime.h>

__global__ void kernel(unsigned char *p)
{
//	int i = blockIdx.x;
//	int j = blockIdx.y;
	int i = threadIdx.x;
	int j = threadIdx.y;
	p[i * 9 + j] = (i + 1) * (j + 1);
}

int main( void )
{
	int devcount = 0;
	cudaError_t error = cudaGetDeviceCount(&devcount);
	if (error != cudaSuccess || devcount < 1) {
		fprintf(stderr, "CUDA device not found\n");
	}

	unsigned char table[81];
	unsigned char *mem;
	cudaMalloc((void **)&mem, 81);
//	dim3 b(9, 9);
	dim3 t(9, 9);
	kernel<<<1,t>>>(mem);
	cudaMemcpy(table, mem, 81, cudaMemcpyDeviceToHost);
	cudaFree(mem);

	for (int i = 0; i < 9; i++) {
		for (int j = 0; j < 9; j++) {
			printf(" %2d", table[i * 9 + j]);
		}
		putchar('\n');
	}

	return 0;
}
