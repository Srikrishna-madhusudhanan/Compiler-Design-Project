int main() {
    int n;
    printf("Enter the size of the array: ");
    scanf("%d", &n);
    n = n + 1;
    int a[n];
    int i;
    for (i = 0; i < n; i = i + 1) {
        a[i] = i * i;
    }
    int sum = 0;
    for (i = 0; i < n; i = i + 1) {
        sum = sum + a[i];
    }
    printf("The sum is %d\n", sum);
    return sum; // Expected: 0+1+4+9+16 = 30
}
