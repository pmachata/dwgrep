struct foo {
  int *f;
  foo (decltype(nullptr)) : f () {}
} x (nullptr);
