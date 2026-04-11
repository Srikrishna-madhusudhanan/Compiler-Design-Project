int main() {
    int x = 10;
    int y = 5;
    int i = 0;
    while (i < 10) {
        x = y + 2; 
        y = y + 2; // common subexpression
        i = i + 1;
    }
    return x;
}
