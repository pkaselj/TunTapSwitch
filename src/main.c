#include <linux/if.h>
#include <linux/if_tun.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include "logger.h"
#include <linux/limits.h>

#define STR_SIZE 256
#define BUFSIZE 1024
#define PACKET_DIR "./packets"


static int do_mkdir(const char *path, mode_t mode);
static int mkpath(const char *path, mode_t mode);
static bool initialize_packets_directory(const char* dir);
static int vswitch_open_device(char *dev);
static char* bytes_to_hexstring(char* bytes, ssize_t size);
static void write_packet_to_file(const char* data, ssize_t size, const char* filename);
static void generate_filename(char* buffer, int max_size, const char* ext, const char* dir);
static void process_packet(char* data, ssize_t size);

int main()
{
    char dev[STR_SIZE];
    memset(dev, 0, sizeof(dev));

    if(initialize_packets_directory(PACKET_DIR) == false)
    {
      exit(-1);
    }

    int fd = vswitch_open_device(dev);
    LOG_Info("Opened device: %s", dev);

    char buffer[BUFSIZE];
    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_read = read(fd, buffer, sizeof(buffer));

        if(bytes_read > 0)
        {
            process_packet(buffer, bytes_read);
        }
        else
        {
            LOG_Warn("Nothing to read.");
        }

        usleep(100000);
    }
    

    return 0;
}

static int vswitch_open_device(char *dev)
{
    struct ifreq ifr;
    int fd;
    int err;

    if((fd = open("/dev/net/tun", O_RDWR)) < 0)
    {
        LOG_Error("Cannot open TUN device! Error: %s", strerror(errno));
        exit(-1);
    }

    LOG_Info("Successfully opened TUN device.");

    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    if (*dev)
    {
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }

    if ((err = ioctl(fd, TUNSETIFF, (void*)&ifr)) < 0 )
    {
        LOG_Error("Error: could not ioctl TUN! Error: %s", strerror(errno));
        close(fd);
        return err;
    }
    
    strcpy(dev, ifr.ifr_name);
    return fd;
    
}

static char hexstring_lookup[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

static char* bytes_to_hexstring(char* bytes, ssize_t size)
{
    size_t hexstring_size = 2 * size + (size - 1) + 1;
    char* hexstring = (char*)malloc(hexstring_size * sizeof(char));

    for (size_t i = 0; i < size; i++)
    {
        char lower_nibble = bytes[i] & 0x0F;
        char upper_nibble = (bytes[i] >> 4) & 0x0F;

        char lower_nibble_char = hexstring_lookup[lower_nibble];
        char upper_nibble_char = hexstring_lookup[upper_nibble];

        if (i == size - 1)
        {
            snprintf((hexstring + 3 * i), size, "%c%c", upper_nibble_char, lower_nibble_char);
        }
        else
        {
            snprintf((hexstring + 3 * i), size, "%c%c:", upper_nibble_char, lower_nibble_char);
        }
    }
    
    return hexstring;
}

static void write_packet_to_file(const char* data, ssize_t size, const char* filename)
{
    int err;
  if(data == NULL || filename == NULL)
  {
    return;
  }

  int fd = open(filename, O_CREAT | O_WRONLY, 0777);
  err = errno;

  if(fd < 0)
  {
    LOG_Error("Could not open %s. Error: %s", filename, strerror(err));
    return;
  }

  ssize_t bytes_written = write(fd, data, size);
  err = errno;
  if (bytes_written <= 0)
  {
    LOG_Error("Could not write packet data to file. Error: %s", strerror(err));
    close(fd);
    return;
  }

  if (bytes_written < size)
  {
    LOG_Warn("Could not write all packet data to file. Written %d of %d bytes", bytes_written, size);
  }
  
  LOG_Info("Successfully written packet to file %s", filename);
  close(fd);
}

static void generate_filename(char* buffer, int max_size, const char* ext, const char* dir)
{
  if(buffer == NULL)
  {
    return;
  }

  time_t rawtime;
  struct tm * timeinfo;

  time (&rawtime);
  timeinfo = localtime (&rawtime);

  char filename[13] = {0};
  strftime(filename, 13, "%Y%m%d%H%M%S", timeinfo);

  srand(rawtime);
  int randint = (rand() % 1000);
  snprintf(buffer, max_size, "%s/%s%03d.%s", dir, filename, randint, ext);
}

static void process_packet(char* data, ssize_t size)
{
  char filename[STR_SIZE] = {0};

  LOG_Debug("Read %d bytes.", size);
  char* hexstring = bytes_to_hexstring(data, size);
  LOG_Debug("%s", hexstring);
  free(hexstring);

  memset(filename, 0, sizeof(filename));
  generate_filename(filename, sizeof(filename), "bin", PACKET_DIR);
  write_packet_to_file(data, size, filename);
}

static bool initialize_packets_directory(const char* dir)
{
  if(dir == NULL)
  {
    return false;
  }

  char abspath[PATH_MAX] = {0};
  if(NULL == realpath(dir, abspath))
  {
    int err = errno;
    LOG_Error("Could not resolve path: '%s'. Error: %s.", dir, strerror(err));
    return false;
  }

  LOG_Debug("Resolved path '%s' to '%s'.", dir, abspath);

  if(mkpath(abspath, 0755) < 0)
  {
    LOG_Warn("Mkpath failed!");
  }

  return true;
}

// https://github.com/dogeel/recdvb/blob/master/mkpath.c
static int do_mkdir(const char *path, mode_t mode)
{
    struct stat st;
    int status = 0;

    if (stat(path, &st) != 0) {
        /* Directory does not exist */
        if (mkdir(path, mode) != 0)
            status = -1;
    }
    else if (!S_ISDIR(st.st_mode)) {
        errno = ENOTDIR;
        status = -1;
    }

    return(status);
}

// https://github.com/dogeel/recdvb/blob/master/mkpath.c
static int mkpath(const char *path, mode_t mode)
{
    char *pp;
    char *sp;
    int status;
    char *copypath = strdup(path);

    status = 0;
    pp = copypath;
    while (status == 0 && (sp = strchr(pp, '/')) != 0) {
        if (sp != pp) {
            /* Neither root nor double slash in path */
            *sp = '\0';
            status = do_mkdir(copypath, mode);
            *sp = '/';
        }
        pp = sp + 1;
    }
    if (status == 0)
        status = do_mkdir(path, mode);
    free(copypath);
    return (status);
}