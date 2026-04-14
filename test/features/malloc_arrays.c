/* Test malloc with arrays and pointer arithmetic */

int main() {
    /* Test 1: Allocate array of integers */
    int *arr = malloc(40);  /* 10 integers * 4 bytes each (assuming 32-bit int) */
    
    /* Test 2: Initialize array elements */
    int i = 0;
    while (i < 5) {
        arr[i] = i * 10;
        i = i + 1;
    }
    
    /* Test 3: Verify array contents */
    i = 0;
    int sum = 0;
    while (i < 5) {
        sum = sum + arr[i];
        i = i + 1;
    }
    
    /* Expected sum: 0 + 10 + 20 + 30 + 40 = 100 */
    if (sum == 100) {
        printf("Sum is correct: %d\n", sum);
        free(arr);
        return 0;  /* Success */
    }
    
    free(arr);
    return 1;  /* Failure */
}
