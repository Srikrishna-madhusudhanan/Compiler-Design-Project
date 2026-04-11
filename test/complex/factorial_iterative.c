int main() {
    int n;
    printf("Enter a positive integer: ");
    scanf("%d", &n);
    int result = 1;
    int k = 1;

    while (k <= n) {
        result = result * k;
        k = k + 1;
    }

    printf("Iterative Factorial of %d is %d\n", n, result);
    return 0;
}