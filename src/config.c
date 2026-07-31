#include "config.h"
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <stdlib.h>

struct server_config sv_conf;

void server_conf_init(struct server_config *config) {
    *config = (struct server_config){
        .port = 8080,
        .root_dir = ".",
        .verbose = 0,
    };
}

static void print_usage(const char *prog) {
    printf("Usage: %s [OPTIONS]\n", prog);
    printf("  -p, --port PORT       Port to open\n");
    printf("  -d, --dir DIRECTORY   Root directory\n");
    printf("  -v, --verbose         Verbose logging (-vv for trace)\n");
    printf("  -h, --help            Show this help\n");
}

void parse_args(int argc, char *argv[], struct server_config *config) {
    static const struct option long_options[] = {
        {"port", required_argument, 0, 'p'},
        {"dir", required_argument, 0, 'd'},
        {"verbose", no_argument, 0, 'v'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0},
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "p:d:vh", long_options, NULL)) != -1) {
        switch (opt) {
        case 'p':
            config->port = atoi(optarg);
            break;
        case 'd':
            // The directory must not contain a trailing slash
            if (optarg[strlen(optarg) - 1] == '/') {
                optarg[strlen(optarg) - 1] = '\0';
            }
            config->root_dir = optarg;
            break;
        case 'v':
            config->verbose += 1;
            break;
        case 'h':
            print_usage(argv[0]);
            exit(EXIT_SUCCESS);
        default:
            print_usage(argv[0]);
            exit(EXIT_FAILURE);
        }
    }
}