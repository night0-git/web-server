#ifndef CONFIG_H
#define CONFIG_H

struct server_config {
    const char *root_dir;
    int port;
    int verbose;
};

extern struct server_config sv_conf;

void server_conf_init(struct server_config *config);

void parse_args(int argc, char *argv[], struct server_config *config);

#endif