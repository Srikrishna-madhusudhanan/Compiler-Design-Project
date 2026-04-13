int main(int x) {
    int a = x + 1;
    int b = x + 2;
    int c = x + 3;
    int d = x + 4;
    int e = x + 5;
    int f = x + 6;
    int g = x + 7;
    int h = x + 8;
    int i = x + 9;
    int j = x + 10;
    
    // many live variables to force spills on 3 registers
    int sum = a + b + c + d + e + f + g + h + i + j;
    
    printf("Sum: %d\n", sum);
    return sum;
}
