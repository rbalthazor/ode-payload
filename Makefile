include Make.rules.arm

override LDFLAGS+=-rdynamic -lproc -lsatpkt -lpolydrivers -lm -lrt -pthread
override CFLAGS+=-Wall -Werror -pedantic -std=gnu99 -g -pthread

SRC=payload.c
OBJS=$(SRC:.c=.o)
EXECUTABLE=payload
CMDS=
INSTALL_DEST=$(BIN_PATH)
CMD_FILE=payload.cmd.cfg

all: $(EXECUTABLE) $(CMDS)

payload: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@ -lz

install: $(EXECUTABLE) $(CMDS)
	cp $(EXECUTABLE) $(INSTALL_DEST)
	$(STRIP) $(INSTALL_DEST)/$(EXECUTABLE)
	cp $(CMD_FILE) $(ETC_PATH)

.PHONY: clean install

clean:
	rm -rf *.o $(EXECUTABLE) $(CMDS)

