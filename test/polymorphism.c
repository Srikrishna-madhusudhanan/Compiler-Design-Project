class Poly{
    public:
    int add (int x, int y){
        return x + y;
    }
    int add (int x, int y, int z)
    {
        return x + y + z;
    }
};

int main()
{
    Poly p;
    int x = 2, y = 4;
    p.add(x, y);
    p.add(1, 2, 3);
    return 0;
}class Base {
public:
    virtual void show() {
        //cout << "Base class\n";
    }
};

class Derived : Base {
public:
    void show(){
        //cout << "Derived class\n";
    }
};

int main() {
    Base* b;
    Derived d;

    b = &d;      // base pointer pointing to derived object
    b->show();   // runtime polymorphism

    return 0;
}