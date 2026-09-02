SRC_DIR := src
INC_DIR := include

EXE := fdtd

SRC := $(wildcard $(SRC_DIR)/*.c)
INC := -I$(SRC_DIR) -I$(INC_DIR)

LIBS := -lm

$(EXE): $(SRC)
	$(CC) $(CFLAGS) $(INC) $(LIBS) -o $(EXE) $^

run: $(EXE)
	./$(EXE)

clean:
	rm -rf $(EXE)

.PHONY: run clean
