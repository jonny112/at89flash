
CC = gcc
CC_OPTS = -Wall -g

BIN = ../bin
BUILD = ../build
OUT = at89flash
IN = $(BUILD)/cmd.o $(BUILD)/main.o $(BUILD)/util.o $(BUILD)/at89dev.o $(BUILD)/ihex.o

all: $(BIN)/$(OUT)

$(BIN)/$(OUT): $(IN)
	$(CC) $(CC_OPTS) -o $@ $^

$(BUILD)/%.o: %.c %.h
	$(CC) $(CC_OPTS) -o $@ -c $<

.PHONY: clean

clean:
	rm -f $(BUILD)/*
	rm -f $(BIN)/*
