#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAXVALUE 255
#define STEP 17

extern char **environ;

void get_brightness_level(char *buf, int value)
{
    int len, filled, i, offset;

    len = MAXVALUE / STEP;
    filled = value / STEP;
    offset = sprintf(buf, "%2d/%2d ", filled, len);

    for (i = 0; i < len; i++) {
        if (i < filled) {
            buf[i + offset] = '+';
        } else {
            buf[i + offset] = '-';
        }
    }
    buf[i + offset] = '\0';
}

int show_notification(int value)
{
    pid_t pid;
    int status;

    pid = fork();
    if (pid < 0) {
        perror("Failed to create child process");
        return 1;
    }

    if (pid == 0) {
        char buf[32];
        char *argv[] = {"/bin/dunstify", "Brightness", buf, "-r", "110001",
                        "-t", "2000", NULL};

        get_brightness_level(buf, value);
        execve(argv[0], argv, environ);
        perror("Failed to send notification");
        return 1;
    }

    wait(&status);
    return WEXITSTATUS(status);
}

void print_usage(FILE *stream)
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
    int value, rc;

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

    rc = fscanf(f, "%d", &value);
    if (rc != 1) {
        fprintf(stderr, "Failed to read brightness value from file\n");
        return 1;
    }

    if (strcmp(*argv, "set") == 0) {
        if (argc < 2) {
            fprintf(stderr, "Not enough arguments\n\n");
            print_usage(stderr);
            return 1;
        }
        argv++;
        value = atoi(*argv);
        if (value > MAXVALUE) value = MAXVALUE;
        if (value < 0) value = 0;
        fprintf(f, "%d", value);
        return 0;
    }

    if (strcmp(*argv, "inc") == 0) {
        value += STEP;
        if (value > MAXVALUE) value = MAXVALUE;
        fprintf(f, "%d", value);
        rc = show_notification(value);
        return rc;
    }

    if (strcmp(*argv, "dec") == 0) {
        value -= STEP;
        if (value < 0) value = 0;
        fprintf(f, "%d", value);
        rc = show_notification(value);
        return rc;
    }

    if (strcmp(*argv, "show") == 0) {
        char buf[32];
        get_brightness_level(buf, value);
        printf("%s\n", buf);
        return 0;
    }

    fprintf(stderr, "Unknown command: %s\n\n", *argv);
    print_usage(stderr);
    return 1;
}
