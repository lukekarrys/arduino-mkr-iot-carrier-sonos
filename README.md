# Arduino MKR IoT Carrier Sonos Controller

## Steps

1. Install `arduino-cli`
1. Create `Sonos/Secrets.h`

    ```cpp
    #define WIFI_SSID "WiFi SSID"
    #define WIFI_PASSWORD "WiFi Password"
    ```

1. Find board `fqbn` and `port` with `arduino-cli board list`
1. Run `./run.sh -b $FQBN -p $PORT`
