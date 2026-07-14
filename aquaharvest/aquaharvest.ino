/*
 * ============================================================================
 *  URJA_ESP32  —  Atmospheric Water Harvester firmware
 * ============================================================================
 *  Reads a DHT22 (temperature + humidity), computes dew point via the Magnus
 *  formula, and streams one CSV line every 3 seconds over the USB serial port
 *  for the AquaHarvest web dashboard (Web Serial API).
 *
 *  Bluetooth is NOT used. All output goes over the regular USB-serial link.
 *
 *  ---------------------------------------------------------------------------
 *  WIRING SUMMARY  (NOTE: the sensor runs on 3.3V, NOT 5V — standard for
 *                   ESP32 dev boards.)
 *  ---------------------------------------------------------------------------
 *
 *   DHT22 (3-pin breakout, built-in pull-up)
 *     VCC      ->  3.3V
 *     GND      ->  GND
 *     DATA/OUT ->  GPIO4
 *
 *  ---------------------------------------------------------------------------
 *  ESP32-SPECIFIC NOTES
 *  ---------------------------------------------------------------------------
 *   * Serial runs at 115200 baud (more reliable than 9600 on the ESP32's
 *     USB-serial bridge). The companion web dashboard is already set to
 *     115200 (its BAUD_RATE constant) to match.
 *
 *  ---------------------------------------------------------------------------
 *  EXACT SERIAL OUTPUT FORMAT (one line every 3 s, no labels/units/extra text)
 *  ---------------------------------------------------------------------------
 *      temp,humidity,dewpoint
 *
 *    temp        DHT22 temperature, °C, 2 decimals
 *    humidity    DHT22 relative humidity, %, 2 decimals
 *    dewpoint    computed dew point, °C, 2 decimals (Magnus formula)
 *
 *    Example valid line:
 *      24.50,58.20,15.80
 * ============================================================================
 */

#include <DHT.h>            // Adafruit "DHT sensor library"
#include <math.h>          // for log() used in the Magnus formula

// ---- Pin / sensor configuration --------------------------------------------
#define DHTPIN       4      // DHT22 DATA/OUT -> GPIO4
#define DHTTYPE      DHT22  // sensor model

// ---- Timing ----------------------------------------------------------------
#define READ_INTERVAL_MS  3000  // one reading every 3 seconds
                                // (DHT22 must not be polled faster than ~2 s)

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  // 115200 baud — the web dashboard's BAUD_RATE must be set to match this.
  Serial.begin(115200);
  dht.begin();
}

void loop() {
  // --- Read the DHT22 -------------------------------------------------------
  float humidity = dht.readHumidity();
  float temp     = dht.readTemperature();  // Celsius by default

  // If either read failed (NaN), skip this cycle entirely — never print
  // malformed data. Just wait and retry on the next loop.
  if (isnan(humidity) || isnan(temp)) {
    delay(READ_INTERVAL_MS);
    return;
  }

  // --- Dew point via the Magnus formula ------------------------------------
  //   a = 17.27, b = 237.7
  //   alpha    = (a * temp) / (b + temp) + ln(humidity / 100)
  //   dewpoint = (b * alpha) / (a - alpha)
  const float a = 17.27;
  const float b = 237.7;
  float alpha    = (a * temp) / (b + temp) + log(humidity / 100.0);
  float dewpoint = (b * alpha) / (a - alpha);

  // --- Emit the CSV line:  temp,humidity,dewpoint ---------------------------
  Serial.print(temp, 2);        // 2 decimal places
  Serial.print(',');
  Serial.print(humidity, 2);    // 2 decimal places
  Serial.print(',');
  Serial.println(dewpoint, 2);  // 2 decimal places + trailing newline

  // --- Wait before the next reading ----------------------------------------
  delay(READ_INTERVAL_MS);
}
