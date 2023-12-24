#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAXVAL 255
#define STEP 17

extern char **environ;
static char display_buf[32];

static void get_brightness_level(int value)
{
    int len, filled, offset;

    len = MAXVAL / STEP;
    filled = value / STEP;
    offset = sprintf(display_buf, "%2d/%2d ", filled, len);

    for (int i = 0; i < len; i++) {
        char c = '-';

        if (i < filled) {
            c = '+';
        }
        display_buf[i + offset] = c;
    }
    display_buf[len + offset] = '\0';
}

static int show_notification(int value)
{
    pid_t pid;
    int status;

    pid = fork();
    if (pid < 0) {
        perror("Failed to create child process");
        return 1;
    }

    if (pid == 0) {
        char *argv[] = {"/bin/dunstify", "Brightness", display_buf, "-r", "2",
                        "-t", "500", NULL};

        get_brightness_level(value);
        execve(argv[0], argv, environ);
        perror("Failed to send notification");
        return 1;
    }

    wait(&status);
    return WEXITSTATUS(status);
}

static void print_usage(FILE *stream)
{
    fprintf(stream, "Usage: backlight-control [COMMAND]\n"
                    "Changes brightness of a screen\n"
                    "\n"
                    "COMMAND:\n"
                    "   set [VALUE] sets new brightness value\n"
                    "   inc         increases brightness\n"
                    "   dec         decreases brightness\n"
                    "   show        shows brightness level\n"
                    "\n"
                    "VALUE:\n"
                    "   integer (0-255)\n");
}

int main(int argc, char **argv)
{
    FILE *f;
    int value;

    argv++;
    argc--;
    if (argc < 1) {
        fprintf(stderr, "Not enough arguments\n\n");
        print_usage(stderr);
        return 1;
    }

    if (strcmp(*argv, "--help") == 0) {
        print_usage(stdout);
        return 0;
    }

    f = fopen("/sys/class/backlight/amdgpu_bl1/brightness", "r+");
    if (f == NULL) {
        perror("Failed to open brightness file");
        return 1;
    }

    if (fscanf(f, "%d", &value) != 1) {
        fprintf(stderr, "Failed to read brightness value\n");
        return 1;
    }

    if (strcmp(*argv, "inc") == 0) {
        value += STEP;
        if (value > MAXVAL) {
            value = MAXVAL;
        }
        fprintf(f, "%d", value);
        return show_notification(value);
    }

    if (strcmp(*argv, "dec") == 0) {
        value -= STEP;
        if (value < 0) {
            value = 0;
        }
        fprintf(f, "%d", value);
        return show_notification(value);
    }

    if (strcmp(*argv, "show") == 0) {
        get_brightness_level(value);
        printf("%s\n", display_buf);
        return 0;
    }

    if (strcmp(*argv, "set") == 0) {
        argv++;
        argc--;
        if (argc < 1) {
            fprintf(stderr, "Not enough arguments\n\n");
            print_usage(stderr);
            return 1;
        }
        value = atoi(*argv);
        if (value > MAXVAL) {
            value = MAXVAL;
        } else if (value < 0) {
            value = 0;
        }
        fprintf(f, "%d", value);
        return show_notification(value);
    }

    fprintf(stderr, "Unknown command: %s\n\n", *argv);
    print_usage(stderr);
    return 1;
}
