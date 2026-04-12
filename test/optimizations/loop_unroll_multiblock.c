int main() {
    int sum = 0;
    int i = 0;

    while (i < 6) {
        if ((i % 2) == 0) {
            sum = sum + i;
        } else {
            sum = sum + (i * 2);
        }
        i = i + 1;
    }

    return sum;
}
