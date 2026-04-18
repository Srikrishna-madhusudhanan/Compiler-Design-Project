class Base { 
    public:
        int pub_x; 
        virtual void func(class Base this)
        { 
            printf("I am Base\n");
        }
        int func2(class Base this)
        { 
            printf("I am Base func2\n");
            return 0;
        } 
    private: 
        int priv_x;
};

class Derived : public Base 
{
    public:
        int derived_x; 
        void func(class Derived this)
        { 
        printf("I am Derived object\n");
        } 
};

int main(){ 
    class Derived d;
    d.pub_x = 10; 
    d.derived_x = 20; 
    //d.priv_x = 30; // Access violation if uncommented 
    d.func(); // Should call method Derived_func
    d.func2(); 
    return 0; 
}
