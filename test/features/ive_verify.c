int main() {
    int sum = 0;
    int i = 0;
    int j = 0;
    for (i = 0; i < 100; i = i + 1) {
        j = i * 4;
        sum = sum + j;
    }
    printf("Sum: %d\n", sum);
    return 0;
}
