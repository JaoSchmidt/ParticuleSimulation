#include <chrono>
#include <iostream>
#include <random>
#include <vector>

using Matrix = std::vector<std::vector<int>>;

// Function to generate a random matrix of given size
Matrix generateRandomMatrix(size_t rows, size_t cols, int minValue,
                            int maxValue) {
  Matrix matrix(rows, std::vector<int>(cols));
  std::random_device rd;  // Seed for random number generation
  std::mt19937 gen(rd()); // Mersenne Twister RNG
  std::uniform_int_distribution<> dis(minValue, maxValue);

  for (size_t i = 0; i < rows; ++i) {
    for (size_t j = 0; j < cols; ++j) {
      matrix[i][j] = dis(gen);
    }
  }

  return matrix;
}

// Function to perform matrix multiplication
int sumElements(const Matrix &A) {
  size_t rowsA = A.size();
  size_t colsA = A[0].size();

  int result;

  for (size_t j = 0; j < colsA; ++j) {
    for (size_t i = 0; i < rowsA; ++i) {
      result += A[i][j];
    }
  }
  return result;
}

int main() {
  // Define dimensions for large matrices
  size_t rowsA = 20000, colsA = 20000;

  // Generate two large random matrices
  Matrix A = generateRandomMatrix(rowsA, colsA, 0, 10);

  // Profile the time for matrix multiplication
  auto start = std::chrono::high_resolution_clock::now();
  int result = sumElements(A);
  auto end = std::chrono::high_resolution_clock::now();

  // Calculate and print the elapsed time
  std::chrono::duration<double> elapsed = end - start;
  std::cout << "Time taken to sum all elements, row first: " << elapsed.count()
            << " seconds" << std::endl;

  return 0;
}
