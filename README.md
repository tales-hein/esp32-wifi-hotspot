# Brief

Simple wifi provisioning webserver and mqtt client implementation using esp-idf.

## Installation

(Note that i only tested this project with an ESP32 dual core devkit and a ESP32C6 (8MB) devkit)

First, make sure you have esp-idf installed and is able to execute scripts using idf.py.
After cloning or downloading the content of this repo you can do the following:

- Set the chip target
```bash
# Configure target for the scripts
idf.py set-target <then follow it up with the target name (esp32c6/esp32)>
```

- Configure partition table and flash size. First open menuconfig using the command below. Then in the "Partition Table" option change the first option to a custom partition. Then change the name of the .csv file acordingly. After this you must change the flash size. To do this you need to go back to the main menu of menuconfig and then select "Serial flasher config" and then change Flash size to 4MB. (Note that if you change targets you will loose the menuconfig configuration, so if you have trouble, when switching between different chips, that may be the problem)
```bash
# Configure using menuconfig
idf.py menuconfig
```

- After setup you can build the project
```bash
# Build
idf.py build
```

- And then flash and monitor (Note that there are many ways to do this part, so you can choose the port etc. if thats your case, but if you only have one esp connected the script is smart enough to find the correct port on its own)
```bash
# Flash and monitor right after, if successful
idf.py flash monitor
```

## Usage

- When you first startup the target with this project flashed to it, you can then access it via wifi. After connection, enter 192.168.4.1 in your browser of choice.
- With this you will be presented with a simple page that lets you inform the name of the network (ssid) and password which the esp will use to connect.
- After a short moment the wifi should be provisioned, and you can test it by listening with your own machine to a locally hosted mqtt broker (i used mosquitto) in the "sources/" topic. Note that, if the connection is successful, the ssid and password will be stored in the esp, so if you lost the message sent to the topic cited above, you can do a quick restart so that the esp boots back up again, reconnects and then publishes to "sources/". The payload that the esp sends to this topic is its mac address, with it you can achieve the next usage case. 
- Another test you can do is publish to the esp. To do so you can send a message to your local broker to this topic: "response/the mac of the esp/". The esp should print the payload you sent to serial output.

## Contributing

Feel free to ask for improvements, help, offer critiques, or even creating pull requests. :)
