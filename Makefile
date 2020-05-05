CC = gcc
CFLAGS = -Wall
PROGRAMS = sender receiver

SENDER_FILES = $(wildcard src/sender/*.c) src/shared/crc32.c

RECEIVER_FILES = $(wildcard src/receiver/*.c) src/shared/crc32.c

ALL: ${PROGRAMS}


sender: ${SENDER_FILES}
	${CC} ${CFLAGS} -pthread -o sender.exe ${SENDER_FILES}

receiver: ${RECEIVER_FILES}
	${CC} ${CFLAGS} -o receiver.exe ${RECEIVER_FILES}

shared: src/shared/crc32.c
	${CC} -g -o shared.exe src/shared/crc32.c

clean:
	rm -f sender.exe receiver.exe shared.exe
