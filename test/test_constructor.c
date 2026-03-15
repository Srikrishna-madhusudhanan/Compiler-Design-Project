class Stack {
    public:
    int top;
    Stack() {
        top = 0;
    }
    ~Stack() {
        top = -1;
    }
};

int main() {
    Stack s;
    return 0;
}
