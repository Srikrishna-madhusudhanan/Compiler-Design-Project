/* Test void* pointer compatibility with malloc and free */

int main() {
    /* Test 1: Assign malloc result to void pointer */
    void *vp = malloc(16);
    
    /* Test 2: Cast void pointer to int pointer and use it */
    int *ip = vp;
    *ip = 123;
    
    /* Test 3: Verify value */
    if (*ip == 123) {
        free(vp);  /* Free using original void* */
        return 0;  /* Success */
    }
    
    /* Test 4: Can also free with the int pointer */
    free(ip);
    return 1;  /* Failure */
}
