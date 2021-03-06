#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>
#include "cdisk.h"

char* helptext = "Usage: ramdisk [options...] <size>\n"
"Options:\n"
" -n, --name    Name the created ramdisk";

void usage() {
  fprintf(stderr, "usage: ramdisk [-n name] size\n");
}

void help() {
  printf("%s\n", helptext);
}

void version() {
  printf("Version: %s\n", VERSION);
}

char *trimwhitespace(char *str) {
  // http://stackoverflow.com/questions/122616/
  char *end;

  // Trim leading space
  while(isspace(*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace(*end)) end--;

  // Write new null terminator
  *(end+1) = 0;

  return str;
}

int main(int argc, char *argv[]) {
  char path[PATH_SIZE];
  char ram[80];
  char* disk_name = "RAM Disk";
  int size, pid, c;
  int fd[2];

  static struct option long_opts[] =
   {
    {"help",     no_argument,         0, 'h'},
    {"version",  no_argument,         0, 'v'},
    {"name",    required_argument,   0, 'n'},
    {0,0,0,0}
   };

  while ((c = getopt_long(argc, argv, "hvn:", long_opts, 0)) != -1) {
    switch(c) {
      case 'h':
        help();
        exit(EXIT_SUCCESS);
      case 'v':
        version();
        exit(EXIT_SUCCESS);
      case 'n':
        disk_name = optarg;
        break;
      default:
        abort();
    }
  }

  if (optind < argc) {
    size = strtol(argv[optind++], NULL, 10);
  } else {
    usage();
    exit(EXIT_FAILURE);
  }

  sprintf(ram, "ram://%d", size*GIGABYTE);

  if(pipe(fd) == -1) {
    perror("pipe");
    exit(EXIT_FAILURE);
  }

  pid = fork();
  if(pid < 0) {
    fprintf(stderr, "Fork failed");
    exit(EXIT_FAILURE);
  } else if(pid == 0) {
    close(fd[0]);
    dup2(fd[1], fileno(stdout));
    execlp("hdiutil", "hdiutil", "attach", "-nomount", ram, NULL);
    exit(EXIT_FAILURE);
  } else {
    close(fd[1]);
    wait(NULL);

    FILE *stream;

    stream = fdopen(fd[0], "r");
    fread(path, PATH_SIZE, 1, stream);
    fclose (stream);
    trimwhitespace(path);
  }

  execlp("diskutil", "diskutil", "erasevolume", "HFS+", disk_name, path, NULL);
  return EXIT_FAILURE;
}
