// g++ -c -g defaulted.cc
// Compiled with GCC 7.2.1
struct Foo {
  Foo () = default;
} foo {};

struct Bar {
  Bar ();
};

Bar::Bar () = default;

Bar bar {};
