#ifndef CONFIG_H
#define CONFIG_H

// backend config
#define BACKEND EDFBackend

#ifdef __APPLE__
#define EDF_BASEDIR "/Users/joshblum/Dropbox (MIT)"
#elif __linux__
#define EDF_BASEDIR "/home/ubuntu"
#endif

// websocket server config
#define WS_NUM_THREADS 4
#define WS_DEFAULT_PORT 8080

#endif // CONFIG_H
