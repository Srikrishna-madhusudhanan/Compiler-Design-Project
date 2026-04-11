int foo(int x, int y) {
    while (x < y) {
        x = x + 1;
        printf("x = %d\n", x + 12);
    }
    return x;
}

int main() {
    int a;
    a = 10;
    foo(a, 20);
    if (a > 5) {
        return a;
    } else {
        return 0;
    }
}

