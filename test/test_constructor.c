class Stack {
    private:
     //int secret;
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
    //s.secret = 5;
    return 0;
}
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
