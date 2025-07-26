#define _POSIX_C_SOURCE 200112L
#define _XOPEN_SOURCE 500

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mta_rand.h>
#include <mta_crypt.h>
#include <stdbool.h>
#include <pthread.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>

#define ENCRYPTER_PIPE "../mnt/mta/encrypter_pipe"
#define DECRYPTER_PIPE_BASE "../mnt/mta/decrypter_pipe_"
#define MAX_DECRYPTER_ID 100
#define MAX_PIPE_NAME 256

char my_pipe_name[MAX_PIPE_NAME];
int my_id = -1;
char encrypted_buffer[256] = {0};
char line[32] = {0};
int password_len =0 ;
int key_len = 0;
int encrypted_length = 0;


void find_and_create_pipe();
void subscribe_to_encrypter();
void recieve_encrypted_password();
void brute_force_and_send_guess(char* encrypted_data, int encrypted_length,int key_len);

int main()
{
    MTA_crypt_init(); // Initialize the MTA crypt library
    find_and_create_pipe(); // Find and create a named pipe for this decrypter
    subscribe_to_encrypter(); // Subscribe to the encrypter
    recieve_encrypted_password(); // Receive the encrypted password and key length from the encrypter
    brute_force_and_send_guess(encrypted_buffer, encrypted_length, key_len); // Start brute-forcing the password and send guesses to the encrypter

    return 0;
}

void find_and_create_pipe()
{
    for(int i = 1 ; i < MAX_DECRYPTER_ID ; i++)
    {
        snprintf(my_pipe_name, MAX_PIPE_NAME, "%s%d", DECRYPTER_PIPE_BASE, i); // Create a unique pipe name for this decrypter
        if(mkfifo(my_pipe_name, 0666) == 0) // Try to create the named pipe
        {
            my_id = i;
            return;
        }
        else if(errno != EEXIST) 
        {
            perror("Failed to create named pipe");
            exit(1);
        }
    }
    fprintf(stderr, "No free decrypter ID found\n");
    exit(1);
}

void subscribe_to_encrypter()
{
    char message[512];
    int fd = open(ENCRYPTER_PIPE, O_WRONLY); // Open the encrypter pipe for writing
    if(fd == -1)
    {
        perror("Failed to open encrypter pipe");
        exit(1);
    }

    int msg_type = 1; // Subscription message type
    int name_len = strlen(my_pipe_name) + 1; 
    write(fd, &msg_type, sizeof(msg_type)); // Write the message type to the pipe
    write(fd, &name_len, sizeof(name_len)); // Write the length of the pipe name
    write(fd, my_pipe_name, name_len); // Write the pipe name to the encrypter
    close(fd);
}

void recieve_encrypted_password()
{
    int fd = open(my_pipe_name, O_RDONLY); // Open the decrypter pipe for reading
    if (fd == -1) {
        perror("Failed to open decrypter pipe");
        exit(1);
    }
    printf("[DECRYPTER %d] Pipe opened: %s\n", my_id, my_pipe_name);

    // Read encrypted_length
    int bytes = read(fd, &encrypted_length, sizeof(encrypted_length));
    if (bytes != sizeof(encrypted_length)) {
        fprintf(stderr, "[DECRYPTER %d] Expected %zu bytes for length, got %d\n",
                my_id, sizeof(encrypted_length), bytes);
        perror("read encrypted_length");
        close(fd);
        exit(1);
    }
    printf("[DECRYPTER %d] encrypted_length = %d\n", my_id, encrypted_length);

    // Read key_len
    bytes = read(fd, &key_len, sizeof(key_len));
    if (bytes != sizeof(key_len)) {
        fprintf(stderr, "[DECRYPTER %d] Expected %zu bytes for key_len, got %d\n",
                my_id, sizeof(key_len), bytes);
        perror("read key_len");
        close(fd);
        exit(1);
    }
    printf("[DECRYPTER %d] key_len = %d\n", my_id, key_len);

    // Read the encrypted data
    int total = 0;
    while (total < encrypted_length) {
        bytes = read(fd, encrypted_buffer + total, encrypted_length - total);
        if (bytes <= 0) {
            perror("read encrypted data");
            close(fd);
            exit(1);
        }
        total += bytes;
    }
    printf("[DECRYPTER %d] Received encrypted payload: %d bytes\n", my_id, total);

    close(fd);
}


void brute_force_and_send_guess(char* encrypted_data, int encrypted_length, int key_len)
{
    char guessed_key[64];
    char decrypted[256];
    unsigned int decrypted_len = 0;
    long iterations = 0;

    while(true)
    {
        memset(guessed_key, 0, sizeof(guessed_key)); // Reset the guessed key
        memset(decrypted, 0, sizeof(decrypted)); // Reset the decrypted output
        MTA_get_rand_data(guessed_key, key_len); // Generate a random key of the specified length

        MTA_decrypt(guessed_key, key_len, encrypted_data, encrypted_length, decrypted, &decrypted_len); // Decrypt the data with the guessed key
        decrypted[decrypted_len] = '\0';

        // Check if the decrypted data is printable
        bool is_printable = true;
        for(int i = 0; i < decrypted_len; i++)
        {
            if(!isprint(decrypted[i]))
            {
                is_printable = false;
                break;
            }
        }

        if(is_printable && decrypted_len > 0) // If the decrypted data is printable and has a non-zero length which means it could be a valid password
        {
            if((iterations % 100000) == 0)
            {
                printf("[CLIENT %d] Iteration %ld\n", my_id, iterations);
            }

            printf("[CLIENT %d] Found candidate #%ld: %.*s (length=%d)\n", my_id, iterations, decrypted_len,decrypted,decrypted_len);


            int out_fd = open(ENCRYPTER_PIPE, O_WRONLY); // Open the encrypter pipe for writing
            if(out_fd != -1) // If the pipe is open, send the guess
            {
                int msg_type = 2; // Guess message type
                int pass_len = decrypted_len; // Length of the decrypted password
                int header = 3* sizeof(int); // Header size for the message (client ID, password length, key length)
                int payload_len = header + pass_len + key_len; // Total payload length
                char payload[payload_len]; // Create a payload buffer to hold the message

                // Fill the payload with the client ID, password length, key length, decrypted password, and guessed key
                int *hdr = (int*)payload;
                hdr[0] = my_id;
                hdr[1] = pass_len;
                hdr[2] = key_len;

                // Copy the decrypted password and guessed key into the payload
                memcpy(payload + header, decrypted, pass_len);
                memcpy(payload + header + pass_len, guessed_key, key_len);

                write(out_fd, &msg_type, sizeof(msg_type));// Write the message type to the pipe
                write(out_fd,&payload_len, sizeof(payload_len)); // Write the total payload length to the pipe
                write(out_fd, payload, payload_len); // Write the payload to the encrypter

                close(out_fd);
            }
            break;
        }
        iterations++;
    }
}

