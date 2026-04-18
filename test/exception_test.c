int main() {
    int x = 10;
    
    printf("Starting try block\n");
    
    try {
        printf("Inside try block, before throw\n");
        if (x > 5) {
            throw 42;
        }
        printf("This should not be printed\n");
    } catch () {
        printf("Caught exception! Value (simulated): %d\n", 42);
    }
    
    printf("After try-catch block\n");
    
    return 0;
}
