struct foo {};

int blah (struct foo f) {
  return 0;
}

int main(int argc, char *argv[])
{
  return blah ((struct foo) {});
}
