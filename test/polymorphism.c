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
}