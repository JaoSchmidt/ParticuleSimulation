
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define N 20000

int columnWise(void)
{
  float *A = (float *)malloc((size_t)N * N * sizeof(float));

  if (!A)
    return 1;

  for (size_t i = 0; i < (size_t)N * N; ++i)
    A[i] = 1.0f;

  float result = 0.0f;

  // Column-wise traversal
  for (size_t j = 0; j < N; ++j) {
    for (size_t i = 0; i < N; ++i) {
      result += A[i * N + j];
    }
  }

  printf("%f\n", result);

  free(A);
  return 0;
}

int main()
{
  columnWise();
  return 0;
}
