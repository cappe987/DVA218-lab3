# TCP-like protocol on top of UDP
```
This program is designed and written by Alexander Andersson, Casper Andersson and Nick Grannas.
```

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
- [X] Make sliding window receiver handle FIN's
- [X] Implement a system to quit.
- [X] Better print messages. Timestamps.
- [X] Limit Sender max window size to 16.
- [X] Global timeout for Receiver Sliding Window.
- [X] Implement go-back-N.
- [X] Auto-send testing system.
- [X] Test with only packet-lost enabled and only packet-corrupt enabled.
- [X] Put header files separately.
- [X] Sender Window: send all data before sending quit and going to teardown.
- [X] Use semaphore for packets_in_window in Sender Window.
- [X] Include metadata in all .c files.
- [ ] Fix bug where windowFront goes too far in Sender Sliding.
- [ ] Write lab report.