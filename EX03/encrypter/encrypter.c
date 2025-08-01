//gcc -o encrypter encrypter.c -lmta_rand -lmta_crypt -lcrypto -Wall

#define CONF_FILE_PATH "../mnt/mta/mtacrypt.conf"
#define SUBSCRIPTION_PIPE_PATH "../mnt/mta/encrypter_pipe"
#include "mta_crypt.h"
#include "mta_rand.h"
#include "crypt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>

struct pass_data
{
    char* password;
    unsigned int length;
    char* key;
    unsigned int key_length;
    char* encrypted_password;
    unsigned int encrypted_password_length;

};

void read_conf_file(struct pass_data *data);
void generate_password_and_key(struct pass_data *data);
void create_subscription_pipe();


int main()
{
    MTA_crypt_init();
    struct pass_data data;
    read_conf_file(&data); // Read configuration file to get password length
    generate_password_and_key(&data); // Generate password and key based on the configuration
    create_subscription_pipe(); // Create the subscription pipe for communication
    int encrypter_fd = open(SUBSCRIPTION_PIPE_PATH, O_RDONLY);
    if (encrypter_fd < 0) {
        perror("Failed to open subscription pipe");
        exit(1);
    }
    
    int decrypter_id;
    read(encrypter_fd, &decrypter_id, sizeof(decrypter_id)); // Read decrypter ID from the pipe
    printf("[SERVER] [INFO] Recieved connection request from decrypter id %d, fifo name /mnt/mta/decrypter_pipe_%d\n", decrypter_id, decrypter_id);
    close(encrypter_fd);

    char decrypter_pipe_name[50];
    sprintf(decrypter_pipe_name, "../mnt/mta/decrypter_pipe_%d", decrypter_id);

    int decrypted_fd = open(decrypter_pipe_name, O_WRONLY);
    if (decrypted_fd < 0) {
        perror("Failed to open decrypter pipe");
        exit(1);
    }

    write(decrypted_fd, &data.encrypted_password_length, sizeof(data.encrypted_password_length));
    write(decrypted_fd, data.encrypted_password, data.encrypted_password_length);
    close(decrypted_fd);
    open(decrypter_pipe_name, O_RDONLY); // Open the pipe to keep it alive for the decrypter to read
    char* decrypter_gueesed_password = malloc(data.length + 1);
    while(true)
    {
        read(decrypted_fd, decrypter_gueesed_password, data.length);
        decrypter_gueesed_password[data.length] = '\0'; // Null-terminate the guessed password

        if (strcmp(decrypter_gueesed_password, data.password) == 0) {
            printf("[SERVER] [OK] Password decrypted successfully by decrypter #%d\n", decrypter_id);
            break; // Exit loop if the password is guessed correctly
        }
    }


    return 0;
}

void read_conf_file(struct pass_data *data)
{
    printf("Reading %s...\n", CONF_FILE_PATH);

    FILE *conf_file = fopen(CONF_FILE_PATH, "r");
    if (!conf_file) {
        perror("Failed to open configuration file");
        exit(1);
    }

    fscanf(conf_file, "PASSWORD_LENGTH=%d", &data->length);
    fclose(conf_file);

    if (data->length <= 0 || data->length % 8 != 0) {
        fprintf(stderr, "Invalid password length specified in configuration file.\n");
        exit(1);
    }

    data->key_length = data->length / 8; // Key length is 1/8th of password length
    printf("Password length set to %d\n", data->length);
}

void generate_password_and_key(struct pass_data *data)
{
    const char *charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    unsigned int charset_size  = strlen(charset);

    data->password = malloc(data->length + 1); // +1 for null terminator
    for(int i = 0 ; i < data->length; i++) {
        data->password[i] = charset[MTA_get_rand_char() % charset_size]; // Generate random index
    }
    data->password[data->length] = '\0'; // Null-terminate the password string

    data->key = malloc(data->key_length); // Allocate memory for key
    for(int i = 0; i < data->key_length; i++) {
        data->key[i] = MTA_get_rand_char(); // Generate random byte for key
    }
    data->key[data->key_length] = '\0'; // Null-terminate the key string

    data->encrypted_password = malloc(data->length + 1); // Allocate memory for encrypted password
    MTA_encrypt(data->key, data->key_length, data->password, data->length, data->encrypted_password, &data->encrypted_password_length);
    data->encrypted_password[data->encrypted_password_length] = '\0'; // Null-terminate the encrypted password string

    printf("[SERVER] [INFO] New password generated: %s, key: %s, After encryption: %s\n", data->password, data->key, data->encrypted_password);
}

void create_subscription_pipe()
{
    if(access(SUBSCRIPTION_PIPE_PATH, F_OK) == 0) {
        // Pipe already exists, remove it
        if (unlink(SUBSCRIPTION_PIPE_PATH) != 0) {
            perror("Failed to remove existing subscription pipe");
            exit(1);
        }
    }

    if (mkfifo(SUBSCRIPTION_PIPE_PATH, 0666) != 0) {
        perror("Failed to create subscription pipe");
        exit(1);
    }
}