#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>

#define DEVICE_PATH "/dev/my_char_device"
#define IOCTL_SET_IO_MODE _IOW('a', 1, int*)
#define IOCTL_GET_LAST_OP_INFO _IOR('a', 2, struct last_operation_info*)

struct last_operation_info {
    unsigned long last_read_time;
    unsigned long last_write_time;
    pid_t read_proc_id;
    pid_t write_proc_id;
    uid_t read_user_id;
    uid_t write_user_id;
};

int main() {
    int mode = 0; // Nonblocking mode
    int fd = open(DEVICE_PATH, O_WRONLY);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }

    if (ioctl(fd, IOCTL_SET_IO_MODE, &mode) < 0) {
        perror("Failed to set nonblocking mode");
        close(fd);
        return 1;
    }

    const char* message = "Message from writer!";
    ssize_t bytes_written = write(fd, message, strlen(message));
    if (bytes_written < 0) {
        perror("Failed to write");
        close(fd);
        return 1;
    }

    printf("Written %zd bytes to %s\n", bytes_written, DEVICE_PATH);

    struct last_operation_info info;
    if (ioctl(fd, IOCTL_GET_LAST_OP_INFO, &info) < 0) {
        perror("Error with ioctl");
        close(fd);
        return 1;
    }

    printf("Last read time: %lu\n", info.last_read_time);
    printf("Last write time: %lu\n", info.last_write_time);
    printf("Last read op process id: %d\n", info.read_proc_id);
    printf("Last write op process id: %d\n", info.write_proc_id);
    printf("Last read op user id: %u\n", info.read_user_id);
    printf("Last write op user id: %u\n", info.write_user_id);

    close(fd);
    return 0;
}