int func(void) {
  return 0;
}


int main(void) {
  int err;
  err = func();

  if (err)
    goto err_out;

  err = func();
  if (err)
    return err;
    
  err = func();
err_out:
  return err;
}
