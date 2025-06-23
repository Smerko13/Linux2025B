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
bool arguments_parser(int argc, char *argv[], int *num_of_decrypters, int *password_length, int *timeout);


struct password_data //shared info for decrypters and encrypters
{
    unsigned int length;
    char* encrypted_password;
    unsigned int encrypted_length;
    int timeout;
    int version;
};

struct password_attempt // used by decrypters to submit gueeses to enrypter
{
    char guessed_password[256];
    char guessed_key[64];
    unsigned int password_len;
    unsigned int key_len;
    int client_id;
    bool has_guess;
};

struct decrypter_data // used to pass individual thread data into each decrypter thread
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
    MTA_crypt_init(); // Initialize the MTA cryptography library

    //Initialize global variables
    current_guess.has_guess = false;
    pass_data.length = 0; 
    pass_data.timeout = 0; // Default timeout
    pass_data.encrypted_password = malloc(1024); // Allocate memory for encrypted password
    if (pass_data.encrypted_password == NULL) 
    {
        printf("Memory allocation failed for encrypted password.\n");
        return;
    }

    // Initialize local variables
    pthread_t encrypter_thread;
    int num_of_decrypters = 0;


    //Helper function to parse and validate command line arguments
    if(!arguments_parser(argc, argv, &num_of_decrypters, &(pass_data.length), &(pass_data.timeout))) 
    {
        printf("Error parsing arguments.\n");
        return;
    }


    // Setting up the decrypters
    pthread_t* decrypter_threads = malloc(num_of_decrypters * sizeof(pthread_t)); // Allocate memory for decrypter threads
    struct decrypter_data* decrypter_data_array = malloc(num_of_decrypters * sizeof(struct decrypter_data)); // Allocate memory for decrypter data. one for each decrypter 
    if (decrypter_threads == NULL || decrypter_data_array == NULL) 
    {
        printf("Memory allocation failed for decrypter threads or data array.\n");
        return;
    }

    // Launching our threads, generating a password and starting decrypter work
    pthread_create(&encrypter_thread, NULL, encrypter, &pass_data); //Starts the encrypter thread
    for(int i = 0; i < num_of_decrypters; i++) 
    {
        decrypter_data_array[i].id = i + 1; // Assigns each thread a unique ID starting from 1 (0 saved for encrypter)
        decrypter_data_array[i].pass_data = &pass_data; // Each decrypter thread will work with the same password data
        pthread_create(&decrypter_threads[i], NULL, decrypter, &decrypter_data_array[i]); // Starts each decrypter thread with its decrypter data struct
    }

    pthread_join(encrypter_thread, NULL);
    for (int i = 0; i < num_of_decrypters; i++) 
    {
        pthread_join(decrypter_threads[i], NULL);
    }

    // Notice: The program will run indefinitely until manually terminated.
    // Cleanup code is not reached, but if it were, it would be placed here.
}

// Function to parse command line arguments, validate them and set the right parameters
// Returns true if parsing is successful, false otherwise.
bool arguments_parser(int argc, char *argv[], int *num_of_decrypters, int *password_length, int *timeout)
{
    for (int i = 1; i < argc; i++) // start from 1 to skip the program name and iterate through each argument
    {
        if (strcmp(argv[i], "-t") == 0 || strcmp (argv[i], "--timeout") == 0) // parse the -t/--timeout flag and its value
        {
            if(i+1 < argc) // check if there is a value after the flag
            {
                *timeout = atoi(argv[++i]);
                if(*timeout <= 0) // check if the timeout is a positive integer
                {
                    printf("Error: Timeout must be a positive integer.\n");
                    return false;
                }
            }
            else  //if there is no value after the flag
            {
                printf("Error: Missing value for -t/--timeout option.\n");
                return false;
            }
        }
        else if (strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--password-length") == 0) // parse the -l/--password-length flag and its value
        {
            if(i+1 < argc) // check if there is a value after the flag
            {
                *password_length = atoi(argv[++i]);
                if(*password_length <= 0) // check if the password length is a positive integer
                {
                    printf("Error: Password length must be a positive integer.\n");
                    return false;
                }
                else if(*password_length%8 != 0 ) // check if the password length is a multiple of 8
                {
                    printf("Error: Password length must be a multiple of 8.\n");
                    return false;
                }
            }
            else // if there is no value after the flag
            {
                printf("Error: Missing value for -l/--password-length option.\n");
                return false;
            }
        }
        else if (strcmp(argv[i], "-n") == 0 || strcmp(argv[i], "--num-of-decrypters") == 0) // parse the -n/--num-of-decrypters flag and its value
        {
            if(i+1 < argc) // check if there is a value after the flag
            {
                *num_of_decrypters = atoi(argv[++i]);
                if(*num_of_decrypters <= 0)  // check if the number of decrypters is a positive integer
                {
                    printf("Error: Number of decrypters must be a positive integer.\n");
                    return false;
                }
            }
            else // if there is no value after the flag
            {
                printf("Error: Missing value for -n/--num-of-decrypters option.\n");
                return false;
            }
        }
        else // if the argument is not recognized
        {
            printf("Error: Unknown option %s\n", argv[i]);
            return false;
        }
    }

    // Validate that we have the basic required parameters
    if (*num_of_decrypters == 0 || *password_length == 0) {
        if(*num_of_decrypters == 0)
            printf("Missing num of decrypters.\n");
        else if(*password_length == 0)
            printf("Missing password length.\n");
        printf("Usage: encrypt.out [-t|--timeout seconds] <-n|--num-of-decrypters <number>> <-l|--password-length <length>>\n");
        return false;
    }

    //If we reach here, all arguments are parsed and valid
    return true;
}

// Encrypter function to generate a random password, encrypt it, and handle guesses from decrypters
void* encrypter(void* pass_data)
{
    int len = ((struct password_data*)pass_data)->length; // retrieves the password length for the shared password_data struct
    int key_len = len / 8; // retrieves the key length, which is 1/8th of the password length
    char password[len + 1]; // A local char array to hold the plain text password, size is set to the password length + 1 for the null terminator
    char key[key_len + 1]; // A local char array to hold the key, size is set to the key length + 1 for the null terminator
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()-_=+[]{}|;:,.<>?"; // A charset to choose printable characters from for the password generation
    unsigned int charset_size = sizeof(charset) - 1; // number of usable characters in the charset (excluding the null terminator)

    while (true) // make encrypter run forever generating, encrypting, dispatching and handling guesses continuesly
    {
        // generate a randon printable password
        for (int i = 0; i < len; i++)
        {
            unsigned char rnd = MTA_get_rand_char();
            password[i] = charset[rnd % charset_size];
        }
        // generate a random key
        for (int i = 0; i < key_len; i++) {
            key[i] = MTA_get_rand_char();
        }
        // Null-terminate the password and key strings
        password[len] = '\0';
        key[key_len] = '\0';

        // Encrypt the password using the key
        MTA_encrypt(key, key_len, password, len,((struct password_data*)pass_data)->encrypted_password,&((struct password_data*)pass_data)->encrypted_length);
        
        // lock the mutex to safely update shared data, broadcast the condition variable to notify decrypters, and print the generated password and key
        pthread_mutex_lock(&mutex);
        ((struct password_data*)pass_data)->version++;
        password_generated = true;
        pthread_cond_broadcast(&cond);
        printf("%ld \t [SERVER]\t [INFO]   New password generated: %s, key: %s, After encryption: ",time(NULL), password, key);
        for (int i = 0; i < ((struct password_data*)pass_data)->encrypted_length; i++) 
        {
            printf("%c", isprint(((struct password_data*)pass_data)->encrypted_password[i]) ? ((struct password_data*)pass_data)->encrypted_password[i] : '.');
        }
        printf("\n");

        // set up optional timeout mechanism in case the timeout is set
        bool timeout_occurred = false;
        struct timespec ts;
        if (((struct password_data*)pass_data)->timeout > 0)
        {
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += ((struct password_data*)pass_data)->timeout;
        }

        // wait for either a guess from a decrypter or a timeout to occur
        while (!is_encrypted && !timeout_occurred)
        {
            while (!current_guess.has_guess) // Wait for a guess from a decrypter
            {
                if (((struct password_data*)pass_data)->timeout > 0) // If a timeout is set, we will wait for a guess or a timeout
                {
                    if (pthread_cond_timedwait(&cond,&mutex , &ts) == ETIMEDOUT) 
                    {
                        timeout_occurred = true;
                        break;
                    }
                } 
                else // If no timeout is set, just wait indefinitely
                {
                    pthread_cond_wait(&cond, &mutex);
                }
            }

            // when timeout occured and no correct guees was recieved:
            if (timeout_occurred)  
            {
                printf("%ld\t [SERVER]\t [ERROR]  Timeout expired (%d sec), regenerating password.\n",time(NULL), ((struct password_data*)pass_data)->timeout);
                break;
            }

            //re-encrypt the password with the guessed key and compare it to the original encrypted password
            char re_encrypted[1024];
            unsigned int re_encrypted_len = 0;
            MTA_encrypt(current_guess.guessed_key, current_guess.key_len,current_guess.guessed_password, current_guess.password_len,re_encrypted, &re_encrypted_len);

            if (re_encrypted_len == ((struct password_data*)pass_data)->encrypted_length && memcmp(re_encrypted, ((struct password_data*)pass_data)->encrypted_password, re_encrypted_len) == 0)
            {
                is_encrypted = true;
                printf("%ld\t [SERVER]\t [OK]\t  Password decrypted successfully by client #%d, received (%s)\n",time(NULL), current_guess.client_id, current_guess.guessed_password);
            }
            else
            {
                printf("%ld\t [SERVER]\t [ERROR] Wrong password received from client #%d (%s), should be (%s)\n",time(NULL), current_guess.client_id, current_guess.guessed_password, password);
            }

            current_guess.has_guess = false; // Reset the global guess state so the encrypter can wait for a new guees
        }

        is_encrypted = false;// reset the is_encrypted flag for the next iteration
        pthread_mutex_unlock(&mutex); // unlocks mutex to complete the password cycle
    }
}

// Decrypter function to guess the password by generating random keys and decrypting the encrypted password
void* decrypter(void* data)
{
    int iterations = 1; // Number of iterations for the decrypter to guess the password
    int my_version = -1; // Version of the password data to ensure synchronization with up to date password of the encrypter
    struct decrypter_data* decrypter_info = (struct decrypter_data*)data; // Cast the input data to decrypter_data struct
    struct password_data* pass_data = decrypter_info->pass_data; // Pointer to the shared password data struct
    int id = decrypter_info->id;  // Unique ID for the current decrypter thread
    int key_len = pass_data->length / 8; // Key length is 1/8th of the password length

    // Initialize buffers for guessed key and decrypted password
    char guessed_key[64];
    char decrypted_password[256];
    char local_encrypted[1024];
    unsigned int decrypted_length = 0;
    unsigned int local_length = 0;

    // Wait for the encrypter to generate the first password
    pthread_mutex_lock(&mutex);
    while (!password_generated) 
    {
        pthread_cond_wait(&cond, &mutex);
    }
    pthread_mutex_unlock(&mutex);

    // inifinite brute force loop to encrypt the password
    while (true)
    {
        //check if a new password has been generated
        pthread_mutex_lock(&mutex);
        if (my_version != pass_data->version) 
        {
            my_version = pass_data->version;
            iterations = 1;
            pthread_mutex_unlock(&mutex);
            continue;
        }
        pthread_mutex_unlock(&mutex);

        // Generate a random key for decryption and attempt to decrypt the password
        iterations++;
        MTA_get_rand_data(guessed_key, key_len);

        pthread_mutex_lock(&mutex);
        local_length = pass_data->encrypted_length;
        memcpy(local_encrypted, pass_data->encrypted_password, local_length);
        pthread_mutex_unlock(&mutex);

        MTA_decrypt(guessed_key, key_len, local_encrypted, local_length,decrypted_password, &decrypted_length);
        decrypted_password[decrypted_length] = '\0';

        //check if fully printable
        bool printable = true;
        for (int i = 0; i < decrypted_length; i++) 
        {
            if (!isprint((unsigned char)decrypted_password[i])) 
            {
                printable = false;
                break;
            }
        }
        if (!printable) {
            continue;
        }

        //submit the decrypters guees if no other guess is pending
        pthread_mutex_lock(&mutex);
        if (!current_guess.has_guess) 
        {
            memcpy(current_guess.guessed_password, decrypted_password, decrypted_length + 1);
            memcpy(current_guess.guessed_key, guessed_key, key_len);
            current_guess.password_len = decrypted_length;
            current_guess.key_len = key_len;
            current_guess.client_id = id;
            current_guess.has_guess = true;

            printf("%ld\t [CLIENT %d]\t [INFO]   After decryption: (%s), key guessed: (%.*s), sending to server after %d iterations\n",time(NULL), id, decrypted_password, key_len, guessed_key, iterations);

            pthread_cond_signal(&cond); // wakes up the encrypter thread to process the guess
        }
        pthread_mutex_unlock(&mutex);
    }
}