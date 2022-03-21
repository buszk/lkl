
int func() {
    return 0;
}

int main() {
    int err;
    mutex_lock();
    err = func();
    mutex_unlock();
    if (err)
        return err;
    return 0;

}