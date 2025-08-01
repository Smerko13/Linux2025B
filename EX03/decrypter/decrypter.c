
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include "mta_crypt.h"
#include "crypt.h"
#include "mta_rand.h"
#include <ctype.h>

#define BASE_DECRYPTER_PATH "../mnt/mta/decrypter_pipe_"
#define ENCRYPTER_PIPE_PATH "../mnt/mta/encrypter_pipe"

struct decrypter_data
{
    int id;
    char pipe_name[50];
    char* password;
    unsigned int length;
    char* key;
    unsigned int key_length;
    char* encrypted_password;
    unsigned int encrypted_password_length;
};

void get_decrypter_pipe_name(struct decrypter_data *data);
int create_decrypter_pipe(struct decrypter_data *data);


int main()
{
    MTA_crypt_init();
    struct decrypter_data data;
    // Check for the next available decrypter pipe name
    get_decrypter_pipe_name(&data);

    // Create the decrypter pipe
    int encrypter_pipe = create_decrypter_pipe(&data);

    write(encrypter_pipe, &(data.id), sizeof(data.id)); // Send the decrypter ID
    close(encrypter_pipe);

     //read password from encrypter
    int decrypter_fd = open(data.pipe_name, O_RDONLY);
    if (decrypter_fd < 0) {
        perror("Failed to open decrypter pipe");
        return 1;
    }
    unsigned int password_length;
    char* encrypted_password;
    read(decrypter_fd, &password_length, sizeof(password_length)); // Read password length
    encrypted_password = malloc(password_length+1);
    read(decrypter_fd, encrypted_password, password_length); // Read encrypted password
    encrypted_password[password_length] = '\0'; // Null-terminate the string
    printf("Received encrypted password: %s\n", encrypted_password);

    char* attempted_password = malloc(password_length + 1);
    int key_length = password_length / 8; // Key length is 1/8th of password length
    char* gueesed_key = malloc(key_length + 1); // Allocate memory for the guessed key

    close(decrypter_fd);
    decrypter_fd = open(data.pipe_name, O_WRONLY);
    int iterations = 1;


    while(true)
    {
        MTA_get_rand_data(gueesed_key, password_length/8); // Generate a random key
        gueesed_key[key_length] = '\0'; // Null-terminate the guessed key
        MTA_decrypt(gueesed_key,key_length, encrypted_password,password_length,attempted_password, &password_length);
        attempted_password[password_length] = '\0'; // Null-terminate the attempted password

        //check if fully printable
        bool printable = true;
        for(int i = 0 ; i < password_length ; i++)
        {
            if(!isprint((unsigned char)attempted_password[i]))
            {
                printable = false;
                break;
            } 
        }
        if(!printable)
        {
            iterations++;
            continue; // Skip to the next iteration if not printable
        }

        
        write(decrypter_fd, attempted_password, password_length); // Write the attempted password

        printf("[CLIENT #%d] [INFO] Decrypted password: %s, Key: %s (in %d iterations)\n", data.id, attempted_password, gueesed_key, iterations);

        iterations++;
    }

}

void get_decrypter_pipe_name(struct decrypter_data *data)
{
    bool foundID=false;
    data->id = 1; // Start with ID 0
    while(!foundID)
    {
        sprintf(data->pipe_name, "%s%d", BASE_DECRYPTER_PATH, data->id);
        if (access(data->pipe_name, F_OK) == 0) {
            data->id++;
        } else {
            foundID=true;
        }
    }
}

int create_decrypter_pipe(struct decrypter_data *data)
{
     if (mkfifo(data->pipe_name, 0666) != 0) {
        perror("Failed to create decrypter pipe");
        return 1;
    }

    int encrypter_pipe = open(ENCRYPTER_PIPE_PATH, O_WRONLY);
    if (!encrypter_pipe) {
        perror("Failed to open encrypter pipe");
        return 1;
    }

    return encrypter_pipe; // Return the file descriptor for the encrypter pipe
}