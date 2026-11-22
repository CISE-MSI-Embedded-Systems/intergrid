Supported on Linux and macOS. On Windows, use WSL2.

## Run ngspice platform

Install ngspice with its shared-library development files before building
(`brew install ngspice` on macOS, `sudo apt install libngspice0-dev` on Ubuntu/WSL2).

```
mkdir -p ./build
cd ./build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
make
./ngspice_platform
```

## Run Arduino platform

Requires `arduino-cli` with the `arduino:avr` core, and `simavr` to emulate
(`brew tap osx-cross/avr && brew install simavr` on macOS, `sudo apt install simavr` on Ubuntu/WSL2).

```
cd ./build
make arduino_hello                                  # build
make arduino_emulate                                # run in simavr (Ctrl-C to quit)
cmake -DARDUINO_PORT=/dev/cu.usbmodemXXXX ..        # port from `arduino-cli board list`
make arduino_upload                                 # flash a real board
```
