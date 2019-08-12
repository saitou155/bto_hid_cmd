# bto_hid_cmd

Bit Trace One commands that run on a general-purpose PC

## Getting Started

This is program for Windows(MinGW)/Ubuntu/OSX.

### Prerequisites

install hidapi for build(Ubuntu/OSX). 

```
sudo apt-get install libhidapi-dev
    or
brew install hidapi
```

## How to use

### Receive infrared remote control

When the reception of the infrared remote control succeeds, it is output to the standard output in Text HEX format.

```
user@DESKTOP-SAHN8DI MINGW32 ~/bto_hid_cmd
$ ./bto_hid_cmd.exe -r
Receive mode
  Received code: C1022080003DBD
```

To save the standard output Text HEX output to a file, do as follows.

```
./bto_hid_cmd.exe -r | tee panasonic_tv_power.txt
```

### Transmit infrared remote control

For infrared remote control transmission, put Text HEX as an argument.

```
user@DESKTOP-SAHN8DI MINGW32 ~/bto_hid_cmd
$ ./bto_hid_cmd.exe -t C1022080003DBD
Transfer mode
  Transfer code: C1022080003DBD
```

To output the Text HEX file with this command, use the shell function as follows.

```
./bto_hid_cmd.exe -t "$(cat panasonic_tv_power.txt)"
      or
./bto_hid_cmd.exe -t "`cat panasonic_tv_power.txt`"
```

## License

This project is Free software.


