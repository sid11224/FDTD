SRC_DIR := src
INC_DIR := include

EXE := fdtd.out

SRC := $(wildcard $(SRC_DIR)/*.c)
INC := -I$(SRC_DIR) -I$(INC_DIR)

LIBS := -lm

$(EXE): $(SRC)
	$(CC) $(CFLAGS) $(INC) $(LIBS) -o $(EXE) $^

clean:
	rm -rf $(EXE)

.PHONY: clean
