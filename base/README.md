# ESP RF Relay Clicker — Base (Receiver)

This is the **base/receiver** unit of the ESP RF Relay Clicker system. It listens for RF24 radio signals and, upon receiving the correct trigger message, activates a relay for a brief pulse. The relay state can also be triggered directly from the built-in web interface.

## Intended Use Cases

- **Garage door opener** — mount the base unit near the opener's existing button terminals and wire the relay across them. A brief relay pulse simulates a button press.
- Any momentary-contact trigger application: gate openers, door locks, bells, or other devices controlled by a dry-contact switch.

## How the Relay Is Triggered

The relay can be activated in two ways:

1. **Web interface** — the home page (`http://<device-ip>/`) displays a **Click Relay** button. Clicking it sends a POST request to the device, which immediately pulses the relay. The button is disabled for the duration of the pulse to prevent double-triggering.

2. **RF24 radio** — a paired transmitter sends a structured 32-byte `RF24Message` packet over the nRF24L01+ radio link. When the base receives a valid message on pipe 0, it identifies the sending remote by its `remoteId` and pulses the relay.

The default pulse duration is **500 ms**, after which the relay automatically returns to its resting state.

## Hardware

Supported platforms:

| Platform | Board |
|---|---|
| ESP8266 | ESP-12E (NodeMCU / Wemos D1) |
| ESP32 | DOIT DevKit v1 |
| ESP32-C3 | ESP32-C3-DevKitM-1 |

Required hardware:
- nRF24L01+ radio module connected via SPI
- Relay module (5 V single-channel)
- *(Optional)* Temperature/humidity sensor (DS18B20, BME280, DHT22, AHT20)
- *(Optional)* Voltage divider on A0 for battery voltage monitoring

## Relay Pin

The relay output pin defaults to:

| Platform | Default Pin |
|---|---|
| ESP8266 | `D1` (GPIO 5) |
| ESP32 / ESP32-C3 | `GPIO 5` |

To use a different pin, edit the `RELAY_PIN` define in `src/Configuration.h`:

```cpp
#define RELAY_PIN D2  // change to any available digital output pin
```

Make sure the pin you choose does not conflict with SPI (used by the RF24 module) or I²C (used by BME280/AHT20).

## RF24 Wiring (SPI)

| Signal | ESP8266 Pin | ESP32 Pin | ESP32-C3 Pin |
|---|---|---|---|
| CE | D4 (GPIO 2) | GPIO 4 | GPIO 4 |
| CSN | D8 (GPIO 15) | GPIO 15 | GPIO 5 |
| SCK | D5 | GPIO 18 | GPIO 3 (default SPI) |
| MOSI | D7 | GPIO 23 | — |
| MISO | D6 | GPIO 19 | — |

## Configuration

### Initial Connection

On first boot (or after a factory reset) the device creates a WiFi soft access point:

- **SSID:** `ESP8266RFRLY_<chip-id>` (or `ESP32RFRLY_<chip-id>`)
- **Password:** `password123`

Connect to this AP and navigate to `http://192.168.4.1/` in a browser to access the web interface.

### Connecting to an Existing WiFi Network

1. Open the web interface and go to **⚙️ → WiFi 🛜**.
2. Enter your network SSID and WPA2 password.
3. Optionally adjust the WiFi TX power.
4. Click **Connect**. The device will reboot and join your network.

If it cannot reach the configured network after three attempts, it falls back to creating its own AP again.

### RF24 Settings

The RF24 radio parameters are configurable from the web interface under **⚙️ → Radio 📻**:

| Setting | Default | Description |
|---|---|---|
| Channel | `76` | RF channel (0–125). Must match the transmitter. |
| Data Rate | `250 Kbps` | `1 Mbps`, `2 Mbps`, or `250 Kbps`. Must match the transmitter. |
| PA Level | `HIGH` (`-6 dBm`) | Transmit power: `MIN`, `LOW`, `HIGH`, `MAX`. |
| Pipe 0 Address | `RFCL0` | Exactly 5 printable ASCII characters. Must match the transmitter. Avoid highly repetitive patterns (e.g. `UUUUU`). |

Changes take effect after the device reboots (triggered automatically on save).

These values can also be changed at compile time in `src/Configuration.h`:

```cpp
#define RF24_CHANNEL       76
#define RF24_DATA_RATE     RF24_250KBPS
#define RF24_PA_LEVEL      RF24_PA_HIGH
#define RF24_PIPE0_ADDRESS "RFCL0"
```

### RF24 Message Format

The receiver expects a packed 32-byte `RF24Message` struct:

| Offset | Size | Field | Description |
|---|---|---|---|
| 0 | 4 B | `header` | Magic bytes `RFCL`. Messages with a different header are discarded with a warning. |
| 4 | 1 B | `remoteId` | ID of the sending remote (1–255). Must match a configured remote entry. |
| 5 | 4 B | `whCode` | Wichmann-Hill rolling code as a `float` in `[0, 1)`. |
| 9 | 4 B | `batteryVoltage` | Remote battery voltage in volts (`float`). |
| 13 | 19 B | `reserved` | Reserved for future telemetry fields. |

Messages with an unrecognised header or an unknown `remoteId` are logged as warnings and ignored.

### Remote Configuration

Up to **5 remotes** can be registered, each with:

- **Remote ID** (1–255) — must match the `remoteId` the transmitter sends.
- **Wichmann-Hill seed components** X, Y, Z (each 1–30000) — initial PRNG state used to validate the rolling code.

Remotes are configured under **⚙️ → Radio 📻**. Each remote row has a **🎲 Randomize** button that generates a fresh random set of seed values. Default seeds are spaced 3000 apart starting at 1000/2000/3000 for remote 1.

Default remote IDs are 1–5.

### MQTT (Optional)

The device can publish sensor readings and receive relay commands over MQTT. Configure the broker address, port, and topic under **⚙️ → Device 📟**.

### Factory Reset

If the device is power-cycled 3 or more times within 2 seconds, it performs a factory reset: all EEPROM configuration is erased and defaults are restored. OTA firmware update is available at `http://<device-ip>/update`.

## Building & Flashing

Built with [PlatformIO](https://platformio.org/). Select the appropriate environment:

```bash
# Build
pio run -e esp8266

# Build and upload (serial)
pio run -e esp8266 --target upload

# Serial monitor
pio device monitor -e esp8266
```

Available environments: `esp8266`, `esp32`, `esp32c3`.

### OTA (Over-the-Air) Update

Once the device is running and connected to WiFi, firmware can be updated wirelessly without a USB connection:

1. Build the firmware for the target environment in PlatformIO.
2. Locate the compiled binary in the build output directory:

   | Environment | Firmware path |
   |---|---|
   | `esp8266` | `.pio/build/esp8266/firmware.bin` |
   | `esp32` | `.pio/build/esp32/firmware.bin` |
   | `esp32c3` | `.pio/build/esp32c3/firmware.bin` |

3. Navigate to `http://<device-ip>/update` in a browser.
4. Select the appropriate `firmware.bin` file and click **Update**.
5. The device will flash the new firmware and reboot automatically.
