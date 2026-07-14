/*
 * ============================================================================
 *  URJA_ESP32  —  Atmospheric Water Harvester firmware
 * ============================================================================
 *  Reads a DHT22 (temperature + humidity), computes dew point via the Magnus
 *  formula, reads a REL_35 analog water-level sensor, and streams one CSV line
 *  every 3 seconds over the USB serial port for the AquaHarvest web dashboard
 *  (Web Serial API).
 *
 *  Bluetooth is NOT used. All output goes over the regular USB-serial link.
 *
 *  ---------------------------------------------------------------------------
 *  WIRING SUMMARY  (NOTE: both sensors run on 3.3V, NOT 5V — standard for
 *                   ESP32 dev boards; feeding an ADC1 pin from a 5V signal
 *                   would exceed the ESP32's 3.3V input limit.)
 *  ---------------------------------------------------------------------------
 *
 *   DHT22 (3-pin breakout, built-in pull-up)
 *     VCC      ->  3.3V
 *     GND      ->  GND
 *     DATA/OUT ->  GPIO4
 *
 *   REL_35 analog water-level sensor (conductivity-based, 3-pin)
 *     VCC      ->  3.3V
 *     GND      ->  GND
 *     Signal   ->  GPIO34
 *
 *  ---------------------------------------------------------------------------
 *  ESP32-SPECIFIC NOTES
 *  ---------------------------------------------------------------------------
 *   * GPIO34 is on ADC1. Use ADC1 pins for analog reads — do NOT use ADC2
 *     pins (e.g. GPIO0/2/4/12-15/25-27) for analog input, because ADC2 is
 *     shared with the WiFi/BT radio and becomes unusable when the radio is on.
 *     GPIO34 is ADC1 and therefore safe.
 *
 *   * GPIO34 is INPUT-ONLY: it has no internal pull-up/pull-down and cannot be
 *     driven as an OUTPUT. That's fine for a passive analog sensor.
 *
 *   * analogRead() on the ESP32 returns a 12-bit value in the range 0..4095
 *     (NOT the 0..1023 range of an Arduino Uno). We print the raw value as-is
 *     with no rescaling — the dashboard just displays the raw number.
 *
 *   * Serial runs at 115200 baud (more reliable than 9600 on the ESP32's
 *     USB-serial bridge). The companion web dashboard is already set to
 *     115200 (its BAUD_RATE constant) to match.
 *
 *  ---------------------------------------------------------------------------
 *  EXACT SERIAL OUTPUT FORMAT (one line every 3 s, no labels/units/extra text)
 *  ---------------------------------------------------------------------------
 *      temp,humidity,dewpoint,waterlevel
 *
 *    temp        DHT22 temperature, °C, 2 decimals
 *    humidity    DHT22 relative humidity, %, 2 decimals
 *    dewpoint    computed dew point, °C, 2 decimals (Magnus formula)
 *    waterlevel  raw analogRead(GPIO34), integer 0..4095, no decimals
 *
 *    Example valid line:
 *      24.50,58.20,15.80,1340
 * ============================================================================
 */

#include <DHT.h>            // Adafruit "DHT sensor library"
#include <math.h>          // for log() used in the Magnus formula

// ---- Pin / sensor configuration --------------------------------------------
#define DHTPIN       4      // DHT22 DATA/OUT -> GPIO4
#define DHTTYPE      DHT22  // sensor model
#define WATER_PIN    34     // REL_35 Signal -> GPIO34 (ADC1, input-only)

// ---- Timing ----------------------------------------------------------------
#define READ_INTERVAL_MS  3000  // one reading every 3 seconds
                                // (DHT22 must not be polled faster than ~2 s)

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  // 115200 baud — the web dashboard's BAUD_RATE must be set to match this.
  Serial.begin(115200);
  dht.begin();

  // GPIO34 is analog input-only; no pinMode(OUTPUT) or pull config is possible
  // or needed here. analogRead() works without any pinMode() call on ESP32.
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

  // --- Read the water-level sensor -----------------------------------------
  // Raw 12-bit ADC value 0..4095, printed as-is (no calibration/mapping).
  int waterlevel = analogRead(WATER_PIN);

  // --- Emit the CSV line:  temp,humidity,dewpoint,waterlevel ----------------
  Serial.print(temp, 2);        // 2 decimal places
  Serial.print(',');
  Serial.print(humidity, 2);    // 2 decimal places
  Serial.print(',');
  Serial.print(dewpoint, 2);    // 2 decimal places
  Serial.print(',');
  Serial.println(waterlevel);   // integer + trailing newline

  // --- Wait before the next reading ----------------------------------------
  delay(READ_INTERVAL_MS);
}
