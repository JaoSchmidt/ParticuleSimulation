
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define N 20000

int rowWise(void)
{
  float *A = (float *)malloc((size_t)N * N * sizeof(float));

  if (!A)
    return 1;

  for (size_t i = 0; i < (size_t)N * N; ++i)
    A[i] = 1.0f;

  float result = 0.0f;

  // Row-wise traversal
  for (size_t i = 0; i < N; ++i) {
    for (size_t j = 0; j < N; ++j) {
      result += A[i * N + j];
    }
  }

  printf("%f\n", result);

  free(A);
  return 0;
}

int main()
{
  rowWise();
  return 0;
}
