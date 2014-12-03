class foo {
public:
  int overload1arg (char);
};


int main ()
{
    foo foo_instance1;
    return 0;
}

int foo::overload1arg (char arg)            { arg = 0; return 2;}

