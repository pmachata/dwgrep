static void foo(void **valp, void *foo) {
  *valp = foo;
}

void foo_wrap(void **valp) {
  foo(valp, 0);
}
