# Twinkle Tracker Firmware
This repository contains all firmware for the twinkle tracker astrophotography
mount. A compiled binary of the latest version is available on the GitHub
releases page, but it is recommended you compile yourself.

## Compiling
Project has only been tested for compilation within a Linux enviroment. Other
host platforms may successfully build the software, but are unsupported.

### Requirements
- ESP-IDF v5.2.1 or compatible.
- Pug
- GZip

### Build Process
To build, simple source the ESP-IDF environment in your shell of choice and use
the `idf.py` tool to compile and flash the code.

```bash
. /opt/esp-idf/export.sh # change path depending on where esp-idf is installed.
idf.py build
idf.py -p /dev/ttyUSB0 flash # alter serial port to match debug adapter.
```
## Project Structure
- 📁 `components` - Contains modular firmware components.
 - 📁 `a4988_driver` - Driver code for A4988 stepper motor driver.
  - 📁 `include`
   - `a4988_driver.h` - A4988 driver public interface.
  - `CMakeLists.txt` - A4988 driver component definition.
  - `a4988_driver.c` - A4988 driver implementation.
   📁 `web_server` - Code implementing REST web server.
  - 📁 `include`
   - `web_server.h` - Web server interface.
  - `CMakeLists.txt` - Web server component definition.
  - `web_server.c` - Web server implementation.
- 📁 `main` - Main component code.
 - 📁 `include` 
  - `network.h` - Declarations of network helper functions.
  - `star_tracker_config.h` - Preprocessor definitions for key device
    configuration properties.
 - `CMakeLists.txt` - Main component definition.
 - `idf_component.yml` - Definition for external mDNS dependency.
 - `main.c` - Firmware entry point.
 - `network.c` - Implementation of network helper functions.
- 📁 `web_front_end` - Front end web page served by firmware.
 - 📁 icons - SVG icons for user interface.
  - `forward.svg` - fast-forward/rewind icon.
  - `play.svg` - play button icon.
  - `star.svg` - star icon.
  - `stop.svg` - stop button icon.
 - `index.js` - Javascript for UI.
 - `index.pug` - Pug file defining structure of HTML for UI.
 - `style.css` - CSS rules for UI.
- `.clang-format` - Formatting rules for project, for use with clang
  format.
- `.clangd` - LSP configuration for use with clangd language server.
- `.gitignore` - Defines temporary files for version control to ignore.
- `CMakeLists.txt` - Base project build configuration.
- `compile_commands.json` - Symlink to `build/compile_commands.json`, which
  is created at build-time. Provides build information for clangd language
  server.
- `dependencies.lock` - IDF external dependency lockfile.
- `sdkconfig.defaults` - Default device configuration for ESP-IDF build
  system.
