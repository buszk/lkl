int main() {
    int err;
    err = func();
    if (err) {
        printf("Something\n");
        return err;
    }
    return 0;
}