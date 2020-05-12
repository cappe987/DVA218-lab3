# TCP-like protocol on top of UDP

## Building and running
### To build sender and receiver:
```
make
```

### To run sender and receiver
Sender and receiver should ideally be run in separate terminal windows to avoid input/outputs colliding.
```
./sender.exe
```
```
./receiver.exe
```

## Debugging in VSCode
Compile the file with `-g` flag. This is currently always set in the `Makefile`.

Create new debugging config with `gdb/c++`. In `launch.json`, set `"program"` to the absolute path to the exe file you want to debug. For example: 
```
"/home/casper/kode/datakomm/3/shared.exe"
```
Remove the `"preLaunchTask"`. After doing changes you need to run `make` again before starting the debugger. Alternatively set up a launch task to run the Makefile.

## base_packet flag value
```
ACK : 1
SYN : 2
FIN : 4
NACK: 8
```
So ACK + SYN = 3. SYN + FIN = 6. etc. It uses the same system as Linux file permissions.

## To-do
- [ ] Make sliding window receiver handle FIN's