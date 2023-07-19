#include "app.h"

int main()
{
    App::Instance().start();
    App::Instance().wait();
    return 0;
}