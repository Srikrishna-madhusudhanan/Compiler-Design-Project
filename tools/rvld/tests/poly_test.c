class Base {
public:
    virtual int show() {
        return 1;
    }
};

class Derived : public Base {
public:
    int show(){
        return 2;
    }
};

int main() {
    Base* b;
    Derived d;

    b = &d;
    return b->show(); // Should return 2
}
