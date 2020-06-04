// примеры ложных срабатываний

#include <memory>

// TODO: пофиксить
int *test1()
{
    int *x = nullptr;
    int *a = new int[2];
    x = a;
    return x; // ложное срабатывания
}

int *test1_1()
{
    int *x = nullptr;
    int *a = new int[2];
    x = a;
    // some work
    return a; // нет ложного срабатывания
}


class A{};

void test2()
{

    A* a = new A();// ложное срабатывание
    std::unique_ptr<A> ptr;
    ptr.reset(a);

    std::unique_ptr<A> ptr2;
    ptr2.reset(new A); // нет ложного срабатывания
    // или
    ptr2 = std::make_unique<A>(); // нет ложного срабатывания
}