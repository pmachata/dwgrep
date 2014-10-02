typedef const volatile unsigned long long int ***W;

W w;

int main (void)
{
}

// gcc -g -c dwz-dupfile.c
// gcc dwz-dupfile.o -o a1.out
// gcc dwz-dupfile.o -o a2.out
// gcc dwz-dupfile.o -o a3.out
// dwz -m dwz-dupfile a{1,2,3}.out
