// g++ -c -g const_value_block.cc

template <class X, bool B> class aaa;

template <class X> class aaa <X, false> { typedef int Y; };
template <class X> class aaa <X, true> { typedef long Y; };

aaa <int*, false> a0;
aaa <int&, true> a1;


template <class X, int B> class bbb;

template <class X> class bbb <X, -7> { typedef int Y; };
template <class X> class bbb <X, 21> { typedef long Y; };

bbb <int*, -7> b0;
bbb <int&, 21> b1;


template <class X, unsigned B> class ccc;

template <class X> class ccc <X, 7> { typedef int Y; };
template <class X> class ccc <X, 3000000000> { typedef long Y; };

ccc <int*, 7> c0;
ccc <int&, 3000000000> c1;


template <class X, signed char B> class ddd;

template <class X> class ddd <X, -7> { typedef int Y; };
template <class X> class ddd <X, 21> { typedef long Y; };

ddd <int*, -7> d0;
ddd <int&, 21> d1;


template <class X, unsigned char B> class eee;

template <class X> class eee <X, 7> { typedef int Y; };
template <class X> class eee <X, 254> { typedef long Y; };

eee <int*, 7> e0;
eee <int&, 254> e1;


template <class X, long long B> class fff;

template <class X> class fff <X, -7> { typedef int Y; };
template <class X> class fff <X, 21> { typedef long Y; };

fff <int*, -7> f0;
fff <int&, 21> f1;


template <class X, unsigned long long B> class ggg;

template <class X> class ggg <X, 7> { typedef int Y; };
template <class X> class ggg <X, 6000000000ULL> { typedef long Y; };

ggg <int*, 7> g0;
ggg <int&, 6000000000ULL> g1;
