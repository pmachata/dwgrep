void
foo32 (char32_t *ptr)
{
  const char32_t bar = U'á';
  *ptr = bar;
}

void
foo16 (char16_t *ptr)
{
  const char16_t bar = u'á';
  *ptr = bar;
}
