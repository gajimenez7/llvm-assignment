void licm_test(int *A, int a, int b, int N) {
  int x;
  for (int i = 0; i < N; i++) {
    /* INVARIANT: a and b never change inside the loop. */
    /* The addition (a + b) and the multiplication (result * 5) should be
     * hoisted. */
    x = (a + b) * 5;
    A[i] = x + i;
  }
}
