CC = gcc
CFLAGS = -Wall
PROGRAMS = sender receiver

SENDER_FILES = $(wildcard src/sender/*.c) src/shared/crc32.c

RECEIVER_FILES = src/receiver/receiver.c

ALL: ${PROGRAMS}


sender: ${SENDER_FILES}
	${CC} ${CFLAGS} -pthread -o sender ${SENDER_FILES}

receiver: ${RECEIVER_FILES}
	${CC} ${CFLAGS} -o receiver ${RECEIVER_FILES}

clean:
	rm -f ${PROGRAMS}
