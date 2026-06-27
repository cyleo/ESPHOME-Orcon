# ESPHOME-Orcon

ESPHome custom component for controlling an **Orcon MVS-15RH** (or compatible Itho/Daalderop) mechanical ventilation unit over **868MHz RF** using a **CC1101** transceiver.

> **This fork has been refactored for ESP32** (ESP-WROOM-32) with an M5Stack CC1101 module.  
> The original codebase targeted the ESP8266 (D1 Mini). See [Credits](#credits) for the original authors.

---

## Hardware

| Component | Details |
|:---|:---|
| **Microcontroller** | ESP-WROOM-32 (38-pin, USB-C, CP2102) |
| **RF Transceiver** | M5Stack CC1101 Module (868MHz, sub-1GHz) |
| **Cables** | 20cm Female-to-Female Dupont wires |

### Wiring (ESP32 VSPI ↔ M5Stack CC1101)

| M5Stack Pin | ESP32 Pin | Function |
|:---|:---|:---|
| `3V3` | `3V3` | 3.3V Power (**never 5V — it will destroy the CC1101**) |
| `GND` | `GND` | Ground |
| `18` | `GPIO18` | SPI SCLK (Clock) |
| `19` | `GPIO19` | SPI MISO |
| `23` | `GPIO23` | SPI MOSI |
| `5` | `GPIO5` | SPI CS (Chip Select) |
| `2` | `GPIO2` | GDO0 (Interrupt) |

### M5Stack DIP Switch Settings

| Switch | Position | Effect |
|:---|:---|:---|
| Switch 1 | **ON** | CSN routed to pin 5 |
| Switch 3 | **ON** | GDO0 routed to pin 2 |

---

## Software Setup

### Prerequisites
- [Home Assistant](https://www.home-assistant.io/) with the [ESPHome Add-on](https://esphome.io/) installed.
- ESPHome 2024.6+ (tested on 2026.6.2).

### Installation

1. **Copy files to Home Assistant:**  
   Copy the following files into your `/config/esphome/` directory on your Home Assistant server:
   - `itho.h`
   - `itho-lib/` (entire folder)
   - `itho.yaml`

   You can do this via the **File Editor** add-on, **Samba share**, or **SCP**.

2. **Configure your IDs in `itho.yaml`:**

   Open `itho.yaml` and look for the `substitutions:` block at the very top. Update the variables to match your setup:

   ```yaml
   substitutions:
     # Your Orcon Unit ID (from the green module sticker, hex to decimal)
     orcon_dest_id_0: "118"
     orcon_dest_id_1: "36"
     orcon_dest_id_2: "148"
     
     # Your physical remote ID (sniffed from ESPHome logs)
     remote1_id: '"11,22,33"'
   ```

3. **Configure WiFi in `itho.yaml`:**

   Replace `<YOURSSID>` and `<YOURPASSWORD>` with your actual WiFi credentials.

4. **Compile and flash** via the ESPHome dashboard.

5. **(Optional) Add the Fan entity to Home Assistant:**  
   Add the contents of `HA_configuration.yaml` under `fan:` in your Home Assistant `configuration.yaml` to get a nice fan speed slider in the UI.

---

## Available Controls

The following switches are exposed to Home Assistant:

| Switch Entity | Orcon Command | State |
|:---|:---|:---|
| `switch.fansendstandby` | Standby | Fan off / lowest |
| `switch.fansendlow` | Low speed | ~1 |
| `switch.fansendmedium` | Medium speed | ~2 |
| `switch.fansendhigh` | High speed | ~3 |
| `switch.fansendauto` | Auto mode | Automatic |

### Text Sensors

| Sensor Entity | Description |
|:---|:---|
| `sensor.fanspeed` | Current fan speed (Low / Medium / High / Auto / Standby) |
| `sensor.fantimer` | Remaining timer countdown (seconds) |
| `sensor.lastid` | Last RF remote ID that changed the state |

---

## Finding Your Orcon & Remote IDs

You must provide the ESP32 with the correct Orcon Destination ID (`destID`) so the ventilation unit accepts its commands. How you find this depends on your current setup:

### Scenario A: You have a physical RF remote (Sniffing method)
If you already have a wireless Orcon RF remote, the ESP32 can sniff the ID directly from the air:
1. In `itho.yaml`, uncomment the verbose logging line:
   ```yaml
   logger:
     level: VERBOSE
   ```
2. Flash the ESP32 and open the Logs viewer.
3. Press any button on your physical Orcon remote.
4. The logs will output: `Intercepted RF Packet from ID: XX,YY,ZZ`.
5. Enter those three numbers into `orcon_dest_id` in your `substitutions:` block! (You can also add it to `remote1_id` to sync state changes).

### Scenario B: You only have a hardwired wall switch
If you do not have an RF remote, there is nothing to sniff. You **must** physically locate the Orcon's factory RF ID sticker.
- The ID is a 6-character hex code (e.g., `76 24 94` or `5AB499`).
- Look for a small white sticker. It is often located on the **underside/back** of the green circuit board inside the unit (requires unscrewing the board), on the outside plastic casing near the cables, or in the original installation manual.
- Once found, convert the 3 hex pairs to decimal (e.g., `76 24 94` hex -> `118 36 148` decimal) and enter them into your `substitutions:` block.

---

## Credits

- Original Itho library by **Klusjesman** and **supersjimmie**.
- Orcon support based on [ESPEasy Plugin 118](https://github.com/letscontrolit/ESPEasy).
- Reworked and extended by **arjenhiemstra**.
- ESP32 refactor and ESPHome 2024+ modernization by **cyleo**.