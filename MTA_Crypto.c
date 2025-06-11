#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mta_rand.h>
#include <mta_crypt.h>
#include <stdbool.h>
#include <pthread.h>

void* encrypter(void* length);
void *decrypter(int length, int timeout);

bool argumnets_parser(int argc, char *argv[], int *num_of_decrypters, int *password_length, int *timeout);

void main(int argc, char *argv[])
{
    pthread_t encrypter_thread, decrypter_thread;
    int num_of_decrypters = 0;
    int password_length = 0;
    int timeout = 0;

    if(!argumnets_parser(argc, argv, &num_of_decrypters, &password_length, &timeout))
    {
        return;
    }

    int encrypter_thread_id = 0;
    pthread_create(&encrypter_thread, NULL, encrypter, &password_length);
    pthread_join(encrypter_thread, NULL);
    int* decrypter_thread_id = malloc(num_of_decrypters * sizeof(int));
    if (decrypter_thread_id == NULL) {
        fprintf(stderr, "Memory allocation failed for decrypter_thread_id.\n");
        return;
    }

    for(int i = 0; i < num_of_decrypters; i++) 
    {
        decrypter_thread_id[i] = i + 1;
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

void* encrypter(void* length)
{
    int len = *((int*)length);
    char password[len + 1];
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()-_=+[]{}|;:,.<>?";
    size_t charset_size = sizeof(charset) - 1; 

    for (int i = 0; i < len; i++) 
    {
        unsigned char rnd = MTA_get_rand_char();
        password[i] = charset[rnd % charset_size];
    }

    password[len] = '\0';

    printf("Generated password: %s\n", password);
}