OBJ = hzy_zip.o
CC = gcc

main: $(OBJ)
	$(CC) -o hzy_zip $(OBJ)

.PHONY: clean
clean:
	rm hzy_zip $(OBJ)
