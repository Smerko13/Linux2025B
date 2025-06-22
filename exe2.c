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


void* encrypter(void* length);
void *decrypter(void* data);
bool argumnets_parser(int argc, char *argv[], int *num_of_decrypters, int *password_length, int *timeout);
bool isPrintable(char* string, int len);


struct password_data
{
    unsigned int length;
    char* encrypted_password;
    unsigned int encrypted_length;
    int timeout;
    int version;
};

struct password_attempt {
    char guessed_password[256];
    char guessed_key[64];
    unsigned int password_len;
    unsigned int key_len;
    int client_id;
    bool has_guess;
};

struct decrypter_data
{
    int id;
    struct password_data* pass_data;
};


struct password_attempt current_guess;

struct password_data pass_data;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
bool is_encrypted = false;
bool password_generated = false;

void main(int argc, char *argv[])
{
    current_guess.has_guess = false;
    MTA_crypt_init();
    pass_data.encrypted_password = malloc(1024); // Allocate memory for encrypted password
    if (pass_data.encrypted_password == NULL) {
        fprintf(stderr, "Memory allocation failed for encrypted password.\n");
        return;
    }
    pthread_t encrypter_thread, decrypter_thread;
    int num_of_decrypters = 0;
    int password_length = 0;
    int timeout = 0;

    if(!argumnets_parser(argc, argv, &num_of_decrypters, &password_length, &(pass_data.timeout))) 
    {
        fprintf(stderr, "Error parsing arguments.\n");
        return;
    }

    pass_data.length = password_length;

    int encrypter_thread_id = 0;
    pthread_create(&encrypter_thread, NULL, encrypter, &pass_data);
    pthread_t* decrypter_threads = malloc(num_of_decrypters * sizeof(pthread_t));
    struct decrypter_data* decrypter_data_array = malloc(num_of_decrypters * sizeof(struct decrypter_data));
    if (decrypter_threads == NULL || decrypter_data_array == NULL) {
        fprintf(stderr, "Memory allocation failed for decrypter threads or data array.\n");
        exit(1);
    }

    for(int i = 0; i < num_of_decrypters; i++) 
    {
        decrypter_data_array[i].id = i + 1;
        decrypter_data_array[i].pass_data = &pass_data;
        pthread_create(&decrypter_threads[i], NULL, decrypter, &decrypter_data_array[i]);
    }

    while(true)
    {
        sleep(1);
    }

}

bool argumnets_parser(int argc, char *argv[], int *num_of_decrypters, int *password_length, int *timeout)
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-t") == 0 || strcmp (argv[i], "--timeout") == 0)
        {
            if(i+1 < argc) 
            {
                *timeout = atoi(argv[++i]);
                if(*timeout <= 0) {
                    fprintf(stderr, "Error: Timeout must be a positive integer.\n");
                    return false;
                }
            }
            else 
            {
                fprintf(stderr, "Error: Missing value for -t/--timeout option.\n");
                return false;
            }
        }
        else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--password-length") == 0)
        {
            if(i+1 < argc)
            {
                *password_length = atoi(argv[++i]);
                if(*password_length <= 0) {
                    fprintf(stderr, "Error: Password length must be a positive integer.\n");
                    return false;
                }
                else if(*password_length%8 != 0 )
                {
                    fprintf(stderr, "Error: Password length must be a multiple of 8.\n");
                    return false;
                }
            }
            else 
            {
                fprintf(stderr, "Error: Missing value for -l/--password-length option.\n");
                return false;
            }
        }
        else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--num-of-decrypters") == 0)
        {
            if(i+1 < argc)
            {
                *num_of_decrypters = atoi(argv[++i]);
                if(*num_of_decrypters <= 0) {
                    fprintf(stderr, "Error: Number of decrypters must be a positive integer.\n");
                    return false;
                }
            }
            else 
            {
                fprintf(stderr, "Error: Missing value for -n/--num-of-decrypters option.\n");
                return false;
            }
        }
        else 
        {
            fprintf(stderr, "Error: Unknown option %s\n", argv[i]);
            return false;
        }
    }
    if (*num_of_decrypters == 0 || *password_length == 0) {
        if(*num_of_decrypters == 0)
            fprintf(stderr, "Misiing num of decrypters.\n");
        else if(*password_length == 0)
            fprintf(stderr, "Missing password length.\n");
        fprintf(stderr, "Usage: encrypt.out [-t|--timeout seconds] <-n|--num-of-decrypters <number>> <-l|--password-length <length>>\n");
        return false;
    }
    return true;
}

void* encrypter(void* pass_data)
{
    int len = ((struct password_data*)pass_data)->length;
    int key_len = len / 8;
    char password[len + 1];
    char key[key_len + 1];
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()-_=+[]{}|;:,.<>?";
    size_t charset_size = sizeof(charset) - 1;

    while (true)
    {
        for (int i = 0; i < len; i++) {
            unsigned char rnd = MTA_get_rand_char();
            password[i] = charset[rnd % charset_size];
        }
        for (int i = 0; i < key_len; i++) {
            key[i] = MTA_get_rand_char();
        }
        password[len] = '\0';
        key[key_len] = '\0';

        MTA_encrypt(key, key_len, password, len,
            ((struct password_data*)pass_data)->encrypted_password,
            &((struct password_data*)pass_data)->encrypted_length);

        pthread_mutex_lock(&mutex);
        ((struct password_data*)pass_data)->version++;
        password_generated = true;
        pthread_cond_broadcast(&cond);

        printf("%ld \t [SERVER]\t [INFO]   New password generated: %s, key: %s, After encryption: ",
               time(NULL), password, key);
        for (int i = 0; i < ((struct password_data*)pass_data)->encrypted_length; i++) {
            printf("%c", isprint(((struct password_data*)pass_data)->encrypted_password[i]) ?
                         ((struct password_data*)pass_data)->encrypted_password[i] : '.');
        }
        printf("\n");

        bool timeout_occurred = false;
        struct timespec ts;

        if (((struct password_data*)pass_data)->timeout > 0) {
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += ((struct password_data*)pass_data)->timeout;
        }

        while (!is_encrypted && !timeout_occurred)
        {
            while (!current_guess.has_guess) {
                if (((struct password_data*)pass_data)->timeout > 0) {
                    if (pthread_cond_timedwait(&cond,&mutex , &ts) == ETIMEDOUT) {
                        timeout_occurred = true;
                        break;
                    }
                } else {
                    pthread_cond_wait(&cond, &mutex);
                }
            }

            if (timeout_occurred) {
                printf("%ld\t [SERVER]\t\t [ERROR]  Timeout expired (%d sec), regenerating password.\n",
                       time(NULL), ((struct password_data*)pass_data)->timeout);
                break;
            }

            // Check the guess
            char re_encrypted[1024];
            unsigned int re_encrypted_len = 0;
            MTA_encrypt(current_guess.guessed_key, current_guess.key_len,
                        current_guess.guessed_password, current_guess.password_len,
                        re_encrypted, &re_encrypted_len);

            if (re_encrypted_len == ((struct password_data*)pass_data)->encrypted_length &&
                memcmp(re_encrypted, ((struct password_data*)pass_data)->encrypted_password, re_encrypted_len) == 0)
            {
                is_encrypted = true;
                printf("%ld\t [SERVER]\t [OK]\t  Password decrypted successfully by client #%d, received (%s)\n",
                       time(NULL), current_guess.client_id, current_guess.guessed_password);
            }
            else
{
            printf("%ld\t [SERVER]\t [ERROR] Wrong password received from client #%d (%s), should be (%s)\n",
            time(NULL), current_guess.client_id, current_guess.guessed_password, password);
}

            current_guess.has_guess = false;
        }

        is_encrypted = false;
        pthread_mutex_unlock(&mutex);
    }
}
void* decrypter(void* data)
{
    int iterations = 1;
    int my_version = -1;
    struct decrypter_data* decrypter_info = (struct decrypter_data*)data;
    struct password_data* pass_data = decrypter_info->pass_data;
    int id = decrypter_info->id;
    int key_len = pass_data->length / 8;

    char guessed_key[64];
    char decrypted_password[256];
    unsigned int decrypted_length = 0;

    pthread_mutex_lock(&mutex);
    while (!password_generated) {
        pthread_cond_wait(&cond, &mutex);
    }
    pthread_mutex_unlock(&mutex);

    while (true)
    {
        pthread_mutex_lock(&mutex);
        if (my_version != pass_data->version) {
            my_version = pass_data->version;
            iterations = 1;
            pthread_mutex_unlock(&mutex);
            continue;
        }
        pthread_mutex_unlock(&mutex);

        iterations++;
        MTA_get_rand_data(guessed_key, key_len);
        MTA_decrypt(guessed_key, key_len, pass_data->encrypted_password,
                    pass_data->encrypted_length, decrypted_password, &decrypted_length);
        decrypted_password[decrypted_length] = '\0';

        bool printable = true;
        for (int i = 0; i < decrypted_length; i++) {
            if (!isprint((unsigned char)decrypted_password[i])) {
                printable = false;
                break;
            }
        }
        if (!printable) {
            continue;
        }

        pthread_mutex_lock(&mutex);
        if (!current_guess.has_guess) {
            memcpy(current_guess.guessed_password, decrypted_password, decrypted_length + 1);
            memcpy(current_guess.guessed_key, guessed_key, key_len);
            current_guess.password_len = decrypted_length;
            current_guess.key_len = key_len;
            current_guess.client_id = id;
            current_guess.has_guess = true;

            printf("%ld\t [CLIENT %d]\t [INFO]   After decryption: (%s), key guessed: (%.*s), sending to server after %d iterations\n",
                   time(NULL), id, decrypted_password, key_len, guessed_key, iterations);

            pthread_cond_signal(&cond);
        }
        pthread_mutex_unlock(&mutex);
    }
}
