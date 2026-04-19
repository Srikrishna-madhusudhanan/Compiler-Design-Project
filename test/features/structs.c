struct S { int a; char b; };
int main() {
    struct S x;
    x.a = 5;
    x.b = 'c';
    printf("%c\n", x.b);
    return x.a;
}
