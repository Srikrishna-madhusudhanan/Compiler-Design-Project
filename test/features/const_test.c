int main() {
    const int MAX = 10;
    int x = MAX;
    int y = x + 1;
    
    // Testing const modification protection
    MAX = 20; // This should throw a semantic error if uncommented
    
    //printf("MAX is %d, x is %d, y is %d\n", MAX, x, y);
    return 0;
}
