SRC_DIR := src
INC_DIR := include

EXE := fdtd.out

SRC := $(wildcard $(SRC_DIR)/*.c)
INC := -I$(SRC_DIR) -I$(INC_DIR)

LOG_LEVEL := "(2)"

LIBS := -lm

$(EXE): $(SRC)
	$(CC) $(CFLAGS) $(INC) $(LIBS) -DLOG_LEVEL=$(LOG_LEVEL) -o $(EXE) $^

clean:
	rm -rf $(EXE)

.PHONY: clean
