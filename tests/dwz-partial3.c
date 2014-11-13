// The intent behind this test is to have a file with a root DIE whose
// offset is the same as an offset of a non-root DIE from the altlink
// file.
typedef const volatile unsigned long long int **const**W;
typedef const volatile long long int ***X;

#if A==1
W w1;

int main (void)
{
}

#elif A==2
W w2;
X x2;

#elif A==3
X x3;

#endif

// gcc -g dwz-partial3.c -c -DA=1 -o dwz-partial3-1.o
// gcc -g dwz-partial3.c -c -DA=2 -o dwz-partial3-2.o
// gcc -g dwz-partial3.c -c -DA=3 -o dwz-partial3-3.o
// gcc -g dwz-partial3-{1,2,3}.o -o dwz-partial3-1
// gcc -g dwz-partial3-{1,2,3}.o -o dwz-partial3-2
// gcc -g dwz-partial3-{1,2,3}.o -o dwz-partial3-3
// dwz -m dwz-partial3-C dwz-partial3-{1,2,3}
