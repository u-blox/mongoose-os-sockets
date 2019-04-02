# Introduction
This is a simple demo application which uses Mongoose OS and a sockets interface to talk to a server on the public internet.

# Prerequisites
To build/download this code you must have the Mongoose OS `mos` tool installed and you must have a board which supports Mongoose OS connected to the USB port of the PC where the tool is installed.  If you wish to build locally, rather than in the cloud, you must also have Docker installed.

# Usage
I used the `mos` tool from the command-line as the GUI failed to work properly (it was unable to exit cleanly on my Windows 10 machine, leaving a stub running which had to be killed manually in the Task Manager, and also flash downloads wouldn't work from  within the GUI).  The command-line console you use MUST be started with "run as admininistrator" as, otherwise, any attempt to run `mos` will cause it to launch a GUI window which exits before you can see what it has done.

To build the code, `cd` to the directory containing this file and run:

```
mos build --platform blah
```

...where `blah` is replaced by your platform name (I used an ESP32 WROOM32 board, hence `blah` was `esp32` for me).  If you have Docker installed and want to build locally, add `--local` to the command-line.

If you are building in the cloud, the build may take a minute or two to complete the first time.  You should see output something like this:

```
Connecting to https://mongoose.cloud, user test
Uploading sources (2582 bytes)
Firmware saved to C:\mos\mongoose-os-sockets\build\fw.zip
```

If you are building locally you will get more verbose output but with the same last line.  Note that if you then switch to building in the cloud you should delete all of the build products first (i.e. the contents of the `build` and `deps` sub-directories) as otherwise `mos` will attempt to upload the lot to the cloud and probably complain that it's too much.

To download this code, note the serial port presented by the board you have connected to your PC and run:

```
mos flash --platform blah --port blahblah
```

..where `blahblah` is replaced by the serial port name, in my case `COM58`.  If you're using an ESP32 board such as mine, you may need to hold a `BOOT` button down on the board, press then release a `RESET` button while the `BOOT` button is held down, then release the `BOOT` button once the flash download procesws has begun.  For my ESP32 board the download process produced the following console output:

```
Loaded mongoose-os-sockets/esp32 version 1.0 (20190321-102107)
Opening COM58 @ 115200...
Connecting to ESP32 ROM, attempt 1 of 10...
Connecting to ESP32 ROM, attempt 2 of 10...
Connecting to ESP32 ROM, attempt 3 of 10...
Connecting to ESP32 ROM, attempt 4 of 10...
  Connected, chip: ESP32D0WDQ6 R1
Running flasher @ 0...
  Flasher is running
Flash size: 4194304, params: 0x022f (dio,32m,80m)
Deduping...
    22832 @ 0x1000 -> 18736
     3072 @ 0x8000 -> 0
    16384 @ 0x9000 -> 8192
     8192 @ 0xd000 -> 0
   262144 @ 0x190000 -> 155648
Writing...
     8192 @ 0x1000
    12288 @ 0x4000
     8192 @ 0x9000
  1564672 @ 0x10000
   155648 @ 0x190000
Wrote 1746592 bytes in 104.34 seconds (130.78 KBit/sec)
Verifying...
    22832 @ 0x1000
     3072 @ 0x8000
    16384 @ 0x9000
     8192 @ 0xd000
  1564016 @ 0x10000
   262144 @ 0x190000
Booting firmware...
All done!
```

To view the debug output from your board, you can run:

```
mos console --port blahblah
```

...but if you want to also enter further commands, without having to run two command windows, it may be easier to run the `mos` tool in GUI mode (just click on it) at this point since it will show console output on the right, command output on the left and a command-line appears at the bottom.  Don't forget to check, when you close the GUI, that it really has exited (check in Task Manager) or it will not work next time.

To connect to Wifi, run the following command:

```
mos wifi SSID PASSWORD --port blahblah
```

...where `SSID` is the SSID of your Wifi network  and `PASSWORD` is the password for your Wifi network.  You should see something like:

```
Getting configuration...
Setting new configuration...
```

If you reset the target board and look at the console output once more you should see that Wifi is now connected e.g.:

```
main.c:67               WiFi standalone connected 0x0.
mgos_net.c:89           WiFi STA: connected
main.c:42               Net connected.
main.c:70               WiFi standalone IP address acquired 0x0.
mgos_net.c:101          WiFi STA: ready, IP 10.20.71.91, GW 10.20.71.254, DNS 195.34.89.241
main.c:45               Net got IP address.
```

If you have a UDP echo server of your own running on a machine on the public internet, edit the parameter `SERVER_ADDRESS` in `main.c` to point to that server or, if you wish, use the u-blox public echo server `udp://echo.u-blox.com:7`.  If it's working you should get console output something like:

```
[Apr  2 12:15:02.582] main.c:200              Main: uptime: 5215 second(s), RAM: 289580 byte(s), 231456 byte(s) free.
[Apr  2 12:15:02.582] main.c:206              Main: Wifi status 3.
[Apr  2 12:15:02.582] main.c:226              Main: sending to socket...
[Apr  2 12:15:02.599] main.c:159              Socket: send (14 byte(s)).
[Apr  2 12:15:02.698] main.c:154              Socket: receive (14 byte(s)).
```
