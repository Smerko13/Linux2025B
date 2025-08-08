CC = gcc
CFLAGS = -Wall -Wno-pointer-sign -Ishared
LDFLAGS = -lcrypto


ENCRYPTER_SRC = encrypter/encrypter.c shared/mta_crypt.c shared/mta_rand.c
DECRYPTER_SRC = decrypter/decrypter.c shared/mta_crypt.c shared/mta_rand.c

DEPS = shared/mta_crypt.h shared/mta_rand.h shared

ENCRYPTER_BIN = encrypter/encrypter.out
DECRYPTER_BIN = decrypter/decrypter.out

.PHONY: all clean

all: $(ENCRYPTER_BIN) $(DECRYPTER_BIN)

$(ENCRYPTER_BIN): $(ENCRYPTER_SRC) $(DEPS)
	$(CC) $(CFLAGS) -o $@ $(ENCRYPTER_SRC) $(LDFLAGS)

$(DECRYPTER_BIN): $(DECRYPTER_SRC) $(DEPS)
	$(CC) $(CFLAGS) -o $@ $(DECRYPTER_SRC) $(LDFLAGS)

clean:
	rm -f $(ENCRYPTER_BIN) $(DECRYPTER_BIN)

