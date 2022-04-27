static void get_afl_input(char* fname) {
    int fd;
    struct stat buf;
    // fprintf(stderr, "%s\n", fname);
    fd = open(fname, O_RDONLY);
    if (fd < 0)
        perror(__func__), exit(1);
    fstat(fd, &buf);
    input_size = buf.st_size;
    if (input_buffer)
        free(input_buffer);
    input_buffer = malloc(input_size);
    if (!input_buffer)
        abort();
    assert(read(fd, input_buffer, input_size) == input_size);
    close(fd);

    // for (int i = 0; i < input_size; i++) {
    //     fprintf(stderr, "%x", ((uint8_t*)input_buffer)[i]);
    // }
    // fprintf(stderr, "\n");
    lkl_set_fuzz_input(input_buffer, input_size);
}

static void load_firmware_disk(void) {
    disk.fd = open("/home/buszk/Workspace/git/lkl/firmware.ext4", O_RDWR);
    assert(disk.fd >= 0);
    disk.ops = NULL;
    disk_id = lkl_disk_add(&disk);;
}
