int main() {
    int err;
    err = func();
    if (err < 0)
        goto err;
err:
    return 0;
}