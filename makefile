CC = gcc
SRC = $(wildcard src/*.c)
OUT = bin/RoseRank

debug:
	$(CC) $(SRC) -Iinclude -std=c11 -Wall -Wextra -Wshadow -g -O0 -mavx -mavx2 -mfma -o $(OUT)Debug

release:
	$(CC) $(SRC) -Iinclude -std=c11 -Wall -Wextra -Wshadow -O3 -mavx -mavx2 -mfma -o $(OUT)