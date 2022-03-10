int func(void) {
  return 0;
}

int main(void) {
  int err;
  err = func();

  if (err)
    goto err_out;
err_out:
  return err;
}
