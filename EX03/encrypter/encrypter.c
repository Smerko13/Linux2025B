#define _POSIX_C_SOURCE 200112L
#define _XOPEN_SOURCE 500

#include <errno.h>
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
#include <sys/stat.h>

#define CONFIG_PATH "../mnt/mta/config.conf"
#define ENCRYPTER_PIPE "../mnt/mta/encrypter_pipe"
#define MAX_PASSWORD_LENGTH 256
#define MAX_KEY_LENGTH 64


struct password_data 
{
    int length;
    int key_length;
    char password[MAX_PASSWORD_LENGTH];
    char key[MAX_KEY_LENGTH];
    char encrypted[MAX_PASSWORD_LENGTH];
    unsigned int encrypted_length;
    int version;
};

struct password_data pass_data;

void load_config();
void generate_and_encrypt();
void create_encrypter_pipe();
void handle_subscriptions_and_guesses();

int main()
{
    MTA_crypt_init(); //Initialize the MTA crypt library
    load_config(); //Load the desired encryption settings based on the config file
    create_encrypter_pipe(); // Create the named pipe for communication with decrypters
    generate_and_encrypt(); // Generate a random password and encrypt it
    handle_subscriptions_and_guesses(); // Handle incoming subscriptions and guesses from decrypters

    return 0;
}

void load_config()
{
    FILE* config = fopen(CONFIG_PATH, "r");
    if(!config)
    {
        perror("Failed to open config file");
        exit(1);
    }

    fscanf(config, "PASSWORD_LENGTH=%d", &pass_data.length);
    fclose(config);

    if(pass_data.length <= 0 || pass_data.length > MAX_PASSWORD_LENGTH || pass_data.length % 8 != 0)
    {
        fprintf(stderr, "Invalid password length in config file\n");
        exit(1);
    }

    pass_data.key_length = pass_data.length / 8;
}

void create_encrypter_pipe()
{
    if(mkfifo(ENCRYPTER_PIPE, 0666) == -1 && errno != EEXIST) // create the pipe and handle the case where it already exists
    {
        perror("Failed to create encrypter pipe");
        exit(1);
    }
}

void generate_and_encrypt()
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    unsigned int charset_size = strlen(charset);

    for(int i = 0; i < pass_data.length; i++)
    {
        pass_data.password[i] = charset[MTA_get_rand_char() % charset_size]; // Generate a random character from the charset and assign it to the password
    }
    pass_data.password[pass_data.length] = '\0';

    for(int i = 0 ; i < pass_data.key_length; i++)
    {
        pass_data.key[i] = MTA_get_rand_char(); // Generate a random character for the key and assign it to the key
    }

    MTA_encrypt(pass_data.key,pass_data.key_length, pass_data.password, pass_data.length, pass_data.encrypted, &pass_data.encrypted_length); // Encrypt the password using the generated key
    pass_data.version++;

    printf("[ENCRYPTER] Password: %s\n", pass_data.password);
    printf("[ENCRYPTER] Encrypted: (%d bytes); ", pass_data.encrypted_length);
    for(int i = 0; i < pass_data.encrypted_length; i++)
    {
        printf("%c", isprint(pass_data.encrypted[i]) ? pass_data.encrypted[i] : '.');
    }
    printf("\n");
}

void handle_subscriptions_and_guesses()
{
    int pipe_fd = open(ENCRYPTER_PIPE, O_RDONLY); // Open the encrypter pipe for reading
    if (pipe_fd == -1) 
    {
         perror("open encrypter pipe");
          exit(1);
 }
    printf("[ENCRYPTER] Listening for messages...\n");

    while (true) {
        int msg_type;
        if (read(pipe_fd, &msg_type, sizeof(int)) != sizeof(int)) // Read the message type from the pipe
        {
            continue;
        }

        if (msg_type == 1) // handle subscription messages
        {
            int name_len;
            read(pipe_fd, &name_len, sizeof(int));
            char client_pipe[name_len];
            read(pipe_fd, client_pipe, name_len);

            printf("[ENCRYPTER] SUBSCRIBE from %s\n", client_pipe);
            int out = open(client_pipe, O_WRONLY); // Open thedecrypter pipe for writing
            if (out == -1) 
            {
                 perror("open client pipe");
                  continue;
            }

            // Send the encrypted password and its length to the subscribed decrypter
            write(out, &pass_data.encrypted_length, sizeof(int));
            write(out, &pass_data.key_length, sizeof(int));
            write(out, pass_data.encrypted, pass_data.encrypted_length);
            close(out);
        }
        else if (msg_type == 2) // handle guess messages
        {
            int client_id, pass_len, key_len;
            // Read the client ID, password length, and key length from the pipe
            read(pipe_fd, &client_id, sizeof(int));
            read(pipe_fd, &pass_len,   sizeof(int));
            read(pipe_fd, &key_len,    sizeof(int));

            char guess_pass[pass_len];
            char guess_key[key_len];
            // Read the guessed password and key from the pipe
            read(pipe_fd, guess_pass, pass_len);
            read(pipe_fd, guess_key,  key_len);
            // null‑terminate for logging
            char tmp_pass[pass_len+1]; 
            memcpy(tmp_pass, guess_pass, pass_len); 
            tmp_pass[pass_len]=0;

            // try decrypt
            char out[MAX_PASSWORD_LENGTH];
            unsigned int out_len = 0;
            MTA_decrypt(guess_key, key_len,pass_data.encrypted, pass_data.encrypted_length,out, &out_len);
            out[out_len]=0;

            if (out_len == pass_data.length && memcmp(out, pass_data.password, pass_data.length)==0) 
            {
                printf("[SERVER] [OK] Client %d cracked: %s\n", client_id, tmp_pass);
            } 
            else 
            {
                printf("[SERVER] [ERR] Client %d bad: %s\n", client_id, tmp_pass);
            }
        }
    }
}
