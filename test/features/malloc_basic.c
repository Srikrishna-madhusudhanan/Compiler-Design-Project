/* Test basic malloc and free functionality */

int main() {
    /* Test 1: Allocate an integer and set value */
    int *p = malloc(8);
    *p = 42;
    *(p + 1) = 99;  /* Write to next int to check for proper allocation */
    
    printf("Value allocated: %d\n", *p);  /* Debug print */
    printf("Next value: %d\n", *(p + 1));  /* Debug print */

    /* Test 2: Verify the value */
    if (*p == 42) {
        return 0;  /* Success */
    }
    
    free(p);
    return 1;  /* Failure */
}
