CC = gcc
CFLAGS = -g -fdiagnostics-color=always
LDFLAGS = -lmta_rand -lmta_crypt -lcrypto

SRC = MTA_Crypto.c
OUT = MTA_Crypto.out

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUT) $(LDFLAGS)

clean:
	rm -f $(OUT)