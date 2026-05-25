/* ABOUTME: Application configuration loaded from STR# resources.
   ABOUTME: Stores proxy address, port, and poll interval. */

#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
    char proxyIP[16];
    int  proxyPort;
    int  pollIntervalSecs;
} AppConfig;

void Config_Load(AppConfig *config);

#endif
