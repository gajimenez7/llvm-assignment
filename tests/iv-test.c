void iv_test(int *A, int N) {
  int k = 0;
  for (int i = 0; i < N; i++) {
    /*
     * k is a derived IV. It follows the pattern {0,+,4}
     * 'k' using 'i' (e.g., i * 4)
     * and remove the dependency on the previous k.
     */
    A[i] = k;
    k = k + 4;
  }
}
