#ifndef CONSTS_UTILS_H
#define CONSTS_UTILS_H

#define NOW millis()

#define MINUTE 60000
#define SLEEP 1800000
#define LOCK 300000
#define WIFI_CHECK 10000
#define SENSOR_CHECK 60000

#define TIMEOUT 1000
#define BACKOFF(count) (1000 * pow(2, count))

#define SUCCESS 200

#define HAPPY_BEEP 800
#define SAD_BEEP 400

#define FAST_BLINK 100

#endif