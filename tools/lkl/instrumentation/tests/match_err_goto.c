int main() {
    int err;
    err = func();
    if (err)
        goto err;
err:
    return 0;
}