#!/bin/bash

# Check argument (if exisst)
if [ $# -ne 1 ]; then
    echo "Usage: ./launcher.sh <number_of_decrypters>"
    exit 1
fi

NUM_DECRYPTERS=$1

# check for valid number given by user in the argument
if ! [[ "$NUM_DECRYPTERS" =~ ^[1-9][0-9]*$ ]]; then
    echo "Error: number_of_decrypters must be a positive integer."
    exit 1
fi

# Cleaning old containers (if any)...
docker rm -f encrypter_container > /dev/null 2>&1
for i in $(seq 1 "$NUM_DECRYPTERS"); do
    docker rm -f "decrypter_container_$i" > /dev/null 2>&1
done

#prepare the needed directories and conf file
sudo mkdir -p /tmp/mtacrypt/
sudo touch /tmp/mtacrypt/mtacrypt.conf
sudo chmod 777 /tmp/mtacrypt/
sudo chmod 666 /tmp/mtacrypt/mtacrypt.conf
sudo echo PASSWORD_LENGTH=16 > /tmp/mtacrypt/mtacrypt.conf

#Creating /var/log files
sudo touch /var/log/encrypter.log
sudo chown $USER:$USER /var/log/encrypter.log
for i in $(seq 1 "$NUM_DECRYPTERS"); do
    sudo touch /var/log/decrypter_$i.log
    sudo chown $USER:$USER /var/log/decrypter_$i.log
done

# Building Docker images...
docker build -f Dockerfile.encrypter -t mta_crypt_encrypter:latest . > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "Error: Failed to build encrypter image."
    exit 1
fi

docker build -f Dockerfile.decrypter -t mta_crypt_decrypter:latest . > /dev/null 2>&1
if [ $? -ne 0 ]; then
    echo "Error: Failed to build decrypter image."
    exit 1
fi


ENCRYPTER_ID=$(docker run -d \
    --name encrypter_container \
    -v /tmp/mtacrypt/:/tmp/mtacrypt/ \
    -v /var/log:/var/log \
    mta_crypt_encrypter:latest)
echo "Encrypter ${ENCRYPTER_ID} starting..."

if [ $? -ne 0 ]; then
    echo "Error: Failed to launch encrypter container."
    exit 1
fi

for i in $(seq 1 "$NUM_DECRYPTERS"); do
DECRYPTER_ID=$(    docker run -d \
        --name "decrypter_container_$i" \
        -v /tmp/mtacrypt/:/tmp/mtacrypt/ \
        -v /var/log:/var/log \
        mta_crypt_decrypter:latest)
    echo "Decrypter #$i(${DECRYPTER_ID}) starting..."

    if [ $? -ne 0 ]; then
        echo "Error: Failed to launch decrypter #$i."
    fi
done
echo "System initialization finished successfully with $1 decrypters"
