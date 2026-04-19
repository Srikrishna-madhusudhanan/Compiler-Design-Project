typedef int myint;
typedef int* intptr;

struct Point {
    int x;
    int y;
};

typedef struct Point PointAlias;

int main() {
    myint a = 1;
    intptr p;
    PointAlias q;

    q.x = a;
    q.y = 2;

    if (q.x + q.y == 3) {
        return 0;
    }

    return 1;
}
