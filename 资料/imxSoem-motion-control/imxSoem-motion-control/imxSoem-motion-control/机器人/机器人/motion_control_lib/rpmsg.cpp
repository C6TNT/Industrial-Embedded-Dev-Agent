#include "rpmsg.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/rpmsg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define DEVICE_CONTROL_NAME "/dev/rpmsg_ctrl0"
#define RPMSG_CLASS_DIR "/sys/class/rpmsg"

static int ep_fd = -1;
static int ctrl_fd = -1;
static int endpoint_created = 0;

static struct rpmsg_endpoint_info ep_info = {
    "rpmsg-virtual-tty-channel-1",
    0x400,
    0x1e,
};

static int read_first_line(const char *path, char *buffer, size_t size)
{
    FILE *file = fopen(path, "r");
    if (file == NULL)
    {
        return -1;
    }

    if (fgets(buffer, (int)size, file) == NULL)
    {
        fclose(file);
        return -1;
    }

    fclose(file);
    buffer[strcspn(buffer, "\r\n")] = '\0';
    return 0;
}

static int find_rpmsg_device(char *device_path, size_t device_path_size, const char *src_match, const char *dst_match)
{
    DIR *dir = opendir(RPMSG_CLASS_DIR);
    if (dir == NULL)
    {
        return -1;
    }

    struct dirent *entry;
    int best_index = -1;

    while ((entry = readdir(dir)) != NULL)
    {
        if (strncmp(entry->d_name, "rpmsg", 5) != 0)
        {
            continue;
        }

        int index = atoi(entry->d_name + 5);
        char path[256];
        char name[128];
        char src[32];
        char dst[32];

        snprintf(path, sizeof(path), "%s/%s/name", RPMSG_CLASS_DIR, entry->d_name);
        if (read_first_line(path, name, sizeof(name)) != 0)
        {
            continue;
        }

        snprintf(path, sizeof(path), "%s/%s/src", RPMSG_CLASS_DIR, entry->d_name);
        if (read_first_line(path, src, sizeof(src)) != 0)
        {
            continue;
        }

        snprintf(path, sizeof(path), "%s/%s/dst", RPMSG_CLASS_DIR, entry->d_name);
        if (read_first_line(path, dst, sizeof(dst)) != 0)
        {
            continue;
        }

        if ((strcmp(name, ep_info.name) == 0) && (strcmp(src, src_match) == 0) && (strcmp(dst, dst_match) == 0))
        {
            if (index > best_index)
            {
                best_index = index;
                snprintf(device_path, device_path_size, "/dev/%s", entry->d_name);
            }
        }
    }

    closedir(dir);
    return best_index >= 0 ? 0 : -1;
}

static int is_matching_endpoint(const char *entry_name)
{
    char path[256];
    char name[128];
    char src[32];
    char dst[32];

    snprintf(path, sizeof(path), "%s/%s/name", RPMSG_CLASS_DIR, entry_name);
    if (read_first_line(path, name, sizeof(name)) != 0)
    {
        return 0;
    }

    snprintf(path, sizeof(path), "%s/%s/src", RPMSG_CLASS_DIR, entry_name);
    if (read_first_line(path, src, sizeof(src)) != 0)
    {
        return 0;
    }

    snprintf(path, sizeof(path), "%s/%s/dst", RPMSG_CLASS_DIR, entry_name);
    if (read_first_line(path, dst, sizeof(dst)) != 0)
    {
        return 0;
    }

    return (strcmp(name, ep_info.name) == 0) &&
           (strcmp(src, "1024") == 0) &&
           (strcmp(dst, "30") == 0);
}

static void destroy_endpoint_device(const char *device_path)
{
    int fd = open(device_path, O_RDWR | O_NONBLOCK);
    if (fd == -1)
    {
        printf("cleanup open %s failed errno=%d\n", device_path, errno);
        return;
    }

    if (ioctl(fd, RPMSG_DESTROY_EPT_IOCTL, NULL) == -1)
    {
        if (errno != ENODEV)
        {
            printf("cleanup destroy %s failed errno=%d\n", device_path, errno);
        }
    }
    else
    {
        printf("cleanup destroy success %s\n", device_path);
    }

    close(fd);
}

static void destroy_stale_endpoints()
{
    DIR *dir = opendir(RPMSG_CLASS_DIR);
    if (dir == NULL)
    {
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strncmp(entry->d_name, "rpmsg", 5) != 0)
        {
            continue;
        }

        if (!is_matching_endpoint(entry->d_name))
        {
            continue;
        }

        char device_path[64];
        snprintf(device_path, sizeof(device_path), "/dev/%s", entry->d_name);
        destroy_endpoint_device(device_path);
    }

    closedir(dir);
}

void rpmsg_close()
{
    if (ep_fd != -1)
    {
        if (endpoint_created && ioctl(ep_fd, RPMSG_DESTROY_EPT_IOCTL, NULL) == -1 && errno != ENODEV)
        {
            printf("destroy active endpoint failed errno=%d\n", errno);
        }
        close(ep_fd);
        ep_fd = -1;
        endpoint_created = 0;
    }

    if (ctrl_fd != -1)
    {
        close(ctrl_fd);
        ctrl_fd = -1;
    }
}

static int32_t rpmsg_flush_buffer()
{
    if (ep_fd == -1)
    {
        return -1;
    }

    int flags = fcntl(ep_fd, F_GETFL, 0);
    if (flags == -1)
    {
        printf("fcntl get flags failed: %d\n", errno);
        return -1;
    }

    if (fcntl(ep_fd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        printf("fcntl set nonblock failed: %d\n", errno);
        return -1;
    }

    char dummy_data[246] = {0x08};
    (void)write(ep_fd, dummy_data, sizeof(dummy_data));
    usleep(100000);

    char buf[1024];
    while (1)
    {
        int bytes_read = read(ep_fd, buf, sizeof(buf));
        if (bytes_read > 0)
        {
            printf("Flush: %d bytes\n", bytes_read);
            continue;
        }
        if (bytes_read == 0)
        {
            break;
        }
        if (errno != EAGAIN && errno != EWOULDBLOCK)
        {
            printf("rpmsg clear: %d\n", errno);
        }
        break;
    }

    if (fcntl(ep_fd, F_SETFL, flags) == -1)
    {
        printf("fcntl restore flags failed: %d\n", errno);
        return -1;
    }

    return 0;
}

int32_t rpmsg_init()
{
    char device_path[64] = {0};

    if (find_rpmsg_device(device_path, sizeof(device_path), "-1", "30") == 0)
    {
        ep_fd = open(device_path, O_RDWR | O_NONBLOCK);
        if (ep_fd != -1)
        {
            printf("RPMSG init using existing channel %s\n", device_path);
            endpoint_created = 0;
            return 0;
        }
        printf("open existing channel %s failed errno=%d\n", device_path, errno);
    }

    destroy_stale_endpoints();
    usleep(100000);

    ctrl_fd = open(DEVICE_CONTROL_NAME, O_RDWR);
    if (ctrl_fd == -1)
    {
        printf("can not open 0\r\n");
        return -1;
    }

    if (ioctl(ctrl_fd, RPMSG_CREATE_EPT_IOCTL, &ep_info) == -1 && errno != EEXIST)
    {
        printf("can not open 1\r\n");
        return -1;
    }
    endpoint_created = 1;

    for (int i = 0; i < 30; ++i)
    {
        if (find_rpmsg_device(device_path, sizeof(device_path), "1024", "30") == 0)
        {
            ep_fd = open(device_path, O_RDWR | O_NONBLOCK);
            if (ep_fd != -1)
            {
                printf("RPMSG init success %s\n", device_path);
                return 0;
            }
            printf("open %s failed errno=%d\n", device_path, errno);
        }
        usleep(100000);
    }

    printf("can not open 2\r\n");
    return -1;
}

int32_t rpmsg_clear()
{
    int32_t ret = rpmsg_flush_buffer();
    printf("RPMSG clear success \n");
    return ret;
}

int32_t rpmsg_write(const char *app_buf, size_t len)
{
    int bytes_written = write(ep_fd, app_buf, len);
    if (bytes_written < 0)
    {
        printf("write_to_serial_port: Failed to write to port\r\n");
    }
    return bytes_written;
}

int rpmsg_read(char *data, size_t len)
{
    int bytes_read = read(ep_fd, data, len);

    if (bytes_read < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return 0;
        }
        printf("rpmsg_read: Failed to read from endpoint: %d\r\n", errno);
        return -1;
    }

    if ((size_t)bytes_read == len)
    {
        return 1;
    }

    printf("Unexpected read size: %d (expected %zu)\n", bytes_read, len);
    return 0;
}
