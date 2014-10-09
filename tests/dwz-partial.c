#if A==1
typedef const volatile unsigned long long int ***W;
W w;

int main (void)
{
}
#elif A==2
typedef const volatile unsigned long long int ***X;
X x;
#elif A==3
typedef const volatile unsigned long long int ***Y;
Y y;
#elif A==4
typedef const volatile unsigned long long int ***Z;
Z z;
#endif

// gcc -g tests/dwz-partial.c -c -DA=1 -o dwz1.o
// gcc -g tests/dwz-partial.c -c -DA=2 -o dwz2.o
// gcc -g tests/dwz-partial.c -c -DA=3 -o dwz3.o
// gcc -g tests/dwz-partial.c -c -DA=4 -o dwz4.o
// gcc dwz{1,2,3,4}.o
// dwz a.out
