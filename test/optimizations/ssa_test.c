int main() {
    int x = 10;
    int y = 20;
    int z;
    if (x > 5) {
        z = x + y;
    } else {
        z = y - x;
    }
    int sum = 0;
    int i = 0;
    while (i < 5) {
        sum = sum + i;
        i = i + 1;
    }
    return z + sum;
}
