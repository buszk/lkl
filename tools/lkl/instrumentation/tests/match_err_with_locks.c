
int func() {
    return 0;
}

int main() {
    int err = 0;
    mutex_lock();
    err = func();
    mutex_unlock();
    if (err)
        goto err;
err:
    return err;

}