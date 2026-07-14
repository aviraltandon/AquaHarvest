# AquaHarvest

Live telemetry dashboard for a Peltier **atmospheric water harvester** — a device that
condenses drinkable water out of the air. An ESP32 reads air temperature, humidity and a
water-level probe, and streams them over USB to a browser dashboard that charts the readings
and logs them to CSV.

Built toward **UN SDG 6 — Clean Water and Sanitation**: harvesting drinkable water from the air, one drop at a time.

## What's in here

| File | Purpose |
|------|---------|
| `aquaharvest/aquaharvest.ino` | ESP32 firmware. Reads the sensors and prints CSV over serial. |
| `water-harvester-dashboard.html` | Self-contained dashboard. Open in Chrome/Edge, connect over USB. |

## Hardware

- ESP32 dev board (DevKitC / WROOM-32 or similar)
- DHT22 temperature + humidity sensor (3-pin breakout with built-in pull-up)
- REL_35 analog water-level sensor (conductivity-based, 3-pin)

### Wiring

> ESP32 runs at **3.3 V logic** — power both sensors from `3V3`, never `5V`. The water-level
> signal feeds straight into the ADC, which is only rated to 3.3 V.

| Sensor pin | ESP32 pin |
|------------|-----------|
| DHT22 VCC | 3V3 |
| DHT22 DATA | GPIO4 |
| DHT22 GND | GND |
| REL_35 VCC | 3V3 |
| REL_35 GND | GND |
| REL_35 Signal | GPIO34 |

GPIO34 is on **ADC1** (safe to use alongside the Wi-Fi/BT radio, unlike ADC2) and is
input-only — exactly right for a passive analog sensor.

## Flashing the firmware

1. Install the [Arduino IDE](https://www.arduino.cc/en/software) and add ESP32 board support
   (Boards Manager → **esp32** by Espressif).
2. Install the **DHT sensor library** by Adafruit (Library Manager) — it will pull in
   **Adafruit Unified Sensor** as a dependency.
3. Open `aquaharvest/aquaharvest.ino`, select your ESP32 board and port, and click **Upload**.

The sketch prints one CSV line every 3 seconds at **115200 baud**:

```
temp,humidity,dewpoint,waterlevel
24.50,58.20,15.80,1340
```

## Running the dashboard

1. Open `water-harvester-dashboard.html` in **Google Chrome or Microsoft Edge**
   (the [Web Serial API](https://developer.mozilla.org/docs/Web/API/Web_Serial_API) it uses
   isn't available in Firefox or Safari).
2. Close the Arduino IDE Serial Monitor if it's open — only one program can hold the port.
3. Click **Connect ESP32** and pick the ESP32's serial port. Live readings appear within a
   couple of seconds.

Everything runs locally in the browser — no server, no internet. You can **Export CSV** at any
time, or stream every reading straight to a file with **Log live to file…**.

## Data format

The dashboard expects four comma-separated fields per line: `temp,humidity,dewpoint,waterlevel`.
The dew point is the temperature the cold plate must drop below before water condenses — the
firmware computes it from temperature and humidity using the Magnus-Tetens approximation.
