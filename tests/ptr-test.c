void ptr_test(double *A, int N) {
    for (int i = 0; i < N; i++) {
        // A[i] implies getelementptr. 
        // The address of A[i] changes by sizeof(double) every iteration.
        // SCEV sees this as {BaseA,+,8}.
        A[i] = 0.0;
    }
}
