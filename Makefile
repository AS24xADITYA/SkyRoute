CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2
SRC_DIR = src
SRCS = $(wildcard $(SRC_DIR)/*.c)
TARGET = skyroute.exe

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	@rm -f $(TARGET) 2>/dev/null || del /f /q $(TARGET) 2>nul || true

.PHONY: all clean
