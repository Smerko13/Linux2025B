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
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <errno.h>

#define BASE_DECRYPTER_PATH "/tmp/mtacrypt/decrypter_pipe_" // Base path for decrypter pipes
#define ENCRYPTER_PIPE_PATH "/tmp/mtacrypt/encrypter_pipe" // Path for encrypter pipe
#define LOG_FILE_BASE_PATH "/var/log/decrypter_" // Base path to log file

struct decrypter_data
{
    int id; // Unique ID for current decrypter
    char pipe_name[50]; // Pipe name for current decrypter
    char log_file_path[100]; // Log file path for this decrypter instance
    char* password; // The current password the decrypter guessed
    unsigned int length; // Length of the password guessed
    char* key; // Key used for decryption
    unsigned int key_length; // Key length
    char* encrypted_password; // Encrypted password received from encrypter
    unsigned int encrypted_password_length; // Length of the encrypted password
    int broadcast_fd; // File descriptor for broadcast pipe, used to receive new passwords
};

void get_decrypter_pipe_name(struct decrypter_data *data);
int create_decrypter_pipe(struct decrypter_data *data);
void open_broadcast_pipe(struct decrypter_data *data);
ssize_t read_full(int fd, void* buffer,size_t length);
ssize_t write_full(int fd, const void* buffer, size_t length);
void log_message(const char* message);
void init_logging(struct decrypter_data *data);
void log_message_for_decrypter(struct decrypter_data *data, const char* message);
bool check_for_new_password(struct decrypter_data *data);


int main()
{
    struct decrypter_data data; // Struct used to hold current guesser data 
    get_decrypter_pipe_name(&data);// Check for the next available decrypter pipe name
    init_logging(&data); // Initialize logging with decrypter ID
    MTA_crypt_init(); // Initialize cryptography library
    int encrypter_pipe = create_decrypter_pipe(&data); // Create the decrypter pipe and open encrypter pipe for writing
    if(write_full(encrypter_pipe, &data.id, sizeof(data.id)) != sizeof(data.id)) // Write the decrypter ID to the encrypter pipe in order to register
    {
        char err_msg[256];
        sprintf(err_msg, "%ld [DECRYPTER] [ERROR] Failed to write decrypter ID to encrypter pipe", time(NULL));
        log_message_for_decrypter(&data, err_msg);
        close(encrypter_pipe);
        exit(1);
    }
    else
    {
        char log_msg[256];
        sprintf(log_msg, "%ld [DECRYPTER %d] [INFO] Sent connect request to encrypter", time(NULL), data.id);
        log_message_for_decrypter(&data, log_msg);
    }
    close(encrypter_pipe); // Close the encrypter pipe after writing the ID
    
    // Open broadcast pipe for listening to new passwords
    open_broadcast_pipe(&data);
    
    //read password from encrypter
    int decrypter_fd = open(data.pipe_name, O_RDONLY);
    if (decrypter_fd < 0) {
        char err_msg[256];
        sprintf(err_msg, "%ld [DECRYPTER %d] [ERROR] Failed to open decrypter pipe", time(NULL), data.id);
        log_message_for_decrypter(&data, err_msg);
        exit(1);
    }
    if(read_full(decrypter_fd, &data.length, sizeof(data.length)) != sizeof(data.length)) // Read the password length
    {
        char err_msg[256];
        sprintf(err_msg, "%ld [DECRYPTER %d] [ERROR] Failed to read password length from decrypter pipe", time(NULL), data.id);
        log_message_for_decrypter(&data, err_msg);
        close(decrypter_fd);
        exit(1);
    }
    if(read_full(decrypter_fd, &data.encrypted_password_length, sizeof(data.encrypted_password_length)) != sizeof(data.encrypted_password_length)) // Read the encrypted password length
    {
        char err_msg[256];
        sprintf(err_msg, "%ld [DECRYPTER %d] [ERROR] Failed to read encrypted password length from decrypter pipe", time(NULL), data.id);
        log_message_for_decrypter(&data, err_msg);
        close(decrypter_fd);
        exit(1);
    }
    data.key_length = data.length / 8; // Key length is 1/8th of password length
    data.encrypted_password = malloc(data.encrypted_password_length + 1); // Allocate memory for the encrypted password
    data.key = malloc(data.key_length + 1); // Allocate memory for the key
    if(read_full(decrypter_fd, data.encrypted_password, data.encrypted_password_length) != data.encrypted_password_length) // Read the encrypted password
    {
        char err_msg[256];
        sprintf(err_msg, "%ld [DECRYPTER %d] [ERROR] Failed to read encrypted password from decrypter pipe", time(NULL), data.id);
        log_message_for_decrypter(&data, err_msg);
        close(decrypter_fd);
        exit(1);
    }
    data.encrypted_password[data.encrypted_password_length] = '\0'; // Null-terminate the string of the encrypted password
    data.password = malloc(data.length + 1); // Allocate memory for the guessed password
    char log_msg[256];
    sprintf(log_msg, "%ld [DECRYPTER %d] [INFO] Recieved encrypted password\n", time(NULL), data.id);
    log_message_for_decrypter(&data, log_msg);

    // Close the read descriptor and reopen for writing guesses
    close(decrypter_fd);
    
    // Open for writing guesses only
    decrypter_fd = open(data.pipe_name, O_WRONLY);
    if (decrypter_fd < 0) {
        char err_msg[256];
        sprintf(err_msg, "%ld [DECRYPTER %d] [ERROR] Failed to reopen decrypter pipe for writing", time(NULL), data.id);
        log_message_for_decrypter(&data, err_msg);
        exit(1);
    }
    
    int iterations = 1;
    while(true)
    {
        // Check for new password from broadcast pipe
        if(check_for_new_password(&data)) {
            char log_msg[256];
            sprintf(log_msg, "%ld [DECRYPTER %d] [INFO] Received new encrypted password %s", time(NULL), data.id, data.encrypted_password);
            log_message_for_decrypter(&data, log_msg);
            // Reset the decryption process
            iterations = 1;
        }
        
        MTA_get_rand_data(data.key, data.key_length); // Generate a random key
        data.key[data.key_length] = '\0'; // Null-terminate the guessed key
        MTA_decrypt(data.key, data.key_length, data.encrypted_password, data.encrypted_password_length, data.password, &data.length);
        data.password[data.length] = '\0'; // Null-terminate the attempted password

        //check if fully printable
        bool printable = true;
        for(int i = 0 ; i < data.length && printable; i++)
        {
            if(!isprint((unsigned char)data.password[i]))
            {
                printable = false;
            } 
        }
        if(!printable)
        {
            iterations++;
            continue; // Skip to the next iteration if not printable
        }
        
        // Send guessed password to encrypter 
        if(write_full(decrypter_fd, data.password, data.length) != data.length) 
        {
            char err_msg[256];
            sprintf(err_msg, "%ld [DECRYPTER %d] [ERROR] Failed to write guessed password to decrypter pipe", time(NULL), data.id);
            log_message_for_decrypter(&data, err_msg);
            close(decrypter_fd);
            exit(1);
        }
        else
        {
            char log_msg[256];
            sprintf(log_msg, "%ld [DECRYPTER %d] [INFO] Decrypted password %s, key %s (in %d iterations)", time(NULL), data.id, data.password, data.key, iterations);
            log_message_for_decrypter(&data, log_msg);
        }

        // Small delay to allow processing
        usleep(1000);
        iterations++;
    }
    
    // Cleanup
    if (data.broadcast_fd >= 0) {
        close(data.broadcast_fd);
    }
    close(decrypter_fd);
    free(data.password);
    free(data.key);
    free(data.encrypted_password);
    
    return 0;
}

//Finds the next id available for the decrypter pipe
void get_decrypter_pipe_name(struct decrypter_data *data)
{
    bool foundID=false;
    data->id = 1; // Start with ID 1
    while(!foundID)
    {
        sprintf(data->pipe_name, "%s%d", BASE_DECRYPTER_PATH, data->id);
        if (access(data->pipe_name, F_OK) == 0) {
            data->id++;
        } else {
            foundID=true;
        }
    }
    // Set the log file path with the decrypter ID
    sprintf(data->log_file_path, "%s%d.log", LOG_FILE_BASE_PATH, data->id);
}

//Create the decrypter pipe and open encrypter pipe for writing
int create_decrypter_pipe(struct decrypter_data *data)
{
     if (mkfifo(data->pipe_name, 0666) != 0) {
        char err_msg[256];
        sprintf(err_msg, "%ld [DECRYPTER] [ERROR] Failed to create decrypter pipe %s", time(NULL), data->pipe_name);
        log_message_for_decrypter(data, err_msg);
        exit(1);
    }

    int encrypter_pipe = open(ENCRYPTER_PIPE_PATH, O_WRONLY);
    if (!encrypter_pipe) {
        char err_msg[256];
        sprintf(err_msg, "%ld [DECRYPTER] [ERROR] Failed to open encrypter pipe", time(NULL));
        log_message_for_decrypter(data, err_msg);
        exit(1);
    }

    return encrypter_pipe; // Return the file descriptor for the encrypter pipe
}

// Open the broadcast pipe for listening to new passwords
void open_broadcast_pipe(struct decrypter_data *data)
{
    // Create and open individual broadcast pipe for this decrypter
    char broadcast_pipe_name[100];
    sprintf(broadcast_pipe_name, "/tmp/mtacrypt/broadcast_pipe_%d", data->id);
    
    // Create the individual broadcast pipe
    if (mkfifo(broadcast_pipe_name, 0666) != 0) {
        if (errno != EEXIST) {
            char err_msg[256];
            sprintf(err_msg, "%ld [DECRYPTER] [ERROR] Failed to create individual broadcast pipe", time(NULL));
            log_message_for_decrypter(data, err_msg);
            exit(1);
        }
    }
    
    // Open broadcast pipe for reading new passwords (non-blocking)
    data->broadcast_fd = open(broadcast_pipe_name, O_RDONLY | O_NONBLOCK);
    if (data->broadcast_fd >= 0) {
        char log_msg[256];
        sprintf(log_msg, "%ld [DECRYPTER] [INFO] Connected to individual broadcast pipe: %s", time(NULL), broadcast_pipe_name);
        log_message_for_decrypter(data, log_msg);
    } else {
        char log_msg[256];
        sprintf(log_msg, "%ld [DECRYPTER] [WARNING] Failed to open individual broadcast pipe, will check periodically", time(NULL));
        log_message_for_decrypter(data, log_msg);
        data->broadcast_fd = -1; // Mark as not connected
    }
}

// Read full data from a file descriptor, retrying until all data is read
// Returns the total number of bytes read, or -1 on error
ssize_t read_full(int fd, void* buffer,size_t length)
{
    size_t total_read = 0;
    while (total_read < length) {
        ssize_t bytes_read = read(fd, (char*)buffer + total_read, length - total_read);
        if (bytes_read < 0) {
            return -1; // Error occurred
        } 
        total_read += bytes_read;
    }
    return total_read; // Return the total number of bytes read
}

// Write full data to a file descriptor, retrying until all data is written
// Returns the total number of bytes written, or -1 on error
ssize_t write_full(int fd, const void* buffer, size_t length)
{
    size_t total_written = 0;
    const char*p = buffer;
    while (total_written < length) {
        ssize_t bytes_written = write(fd, p + total_written, length - total_written);
        if (bytes_written < 0) {
            return -1; // Error occurred
        }
        total_written += bytes_written;
    }
    return total_written; // Return the total number of bytes written
}

// Initialize logging to syslog and file
void init_logging(struct decrypter_data *data)
{
    char syslog_name[50];
    sprintf(syslog_name, "decrypter_%d", data->id);
    openlog(syslog_name, LOG_PID | LOG_CONS, LOG_USER);
}

// Log a message to syslog and a log file for a specific decrypter
void log_message_for_decrypter(struct decrypter_data *data, const char* message)
{
    // Log to syslog
    syslog(LOG_INFO, "%s", message);
    
    // Also log to the specific decrypter log file
    FILE* log_file = fopen(data->log_file_path, "a");
    if (log_file) {
        fprintf(log_file, "%s\n", message);
        fclose(log_file);
    }
}

// Legacy log function - keep for compatibility
void log_message(const char* message)
{
    // Log to syslog
    syslog(LOG_INFO, "%s", message);
    
    // Also log to generic file (fallback)
    FILE* log_file = fopen("/var/log/decrypter_generic.log", "a");
    if (log_file) {
        fprintf(log_file, "%s\n", message);
        fclose(log_file);
    }
}

// Check for new password from broadcast pipe
// If a new password is received, update the decrypter data and return true
// If no new password is available, return false
bool check_for_new_password(struct decrypter_data *data)
{
    // If broadcast pipe is not open, try to open it
    if (data->broadcast_fd < 0) {
        char broadcast_pipe_name[100];
        sprintf(broadcast_pipe_name, "/tmp/mtacrypt/broadcast_pipe_%d", data->id);
        data->broadcast_fd = open(broadcast_pipe_name, O_RDONLY | O_NONBLOCK);
        if (data->broadcast_fd < 0) {
            return false; // Still can't open it
        }
    }
    
    unsigned int new_length;
    unsigned int new_encrypted_length;
    
    // Try to read new password information from the persistent file descriptor
    ssize_t bytes_read = read(data->broadcast_fd, &new_length, sizeof(new_length));
    if (bytes_read == sizeof(new_length)) {
        // Read encrypted password length
        if (read(data->broadcast_fd, &new_encrypted_length, sizeof(new_encrypted_length)) == sizeof(new_encrypted_length)) {
            // Allocate memory for new encrypted password
            char* new_encrypted = malloc(new_encrypted_length + 1);
            if (read(data->broadcast_fd, new_encrypted, new_encrypted_length) == new_encrypted_length) {
                new_encrypted[new_encrypted_length] = '\0';
                
                // Update data with new password
                free(data->encrypted_password);
                data->encrypted_password = new_encrypted;
                data->encrypted_password_length = new_encrypted_length;
                data->length = new_length;
                data->key_length = new_length / 8;
                
                // Reallocate memory for password and key if needed
                free(data->password);
                free(data->key);
                data->password = malloc(data->length + 1);
                data->key = malloc(data->key_length + 1);
                
                char log_msg[256];
                sprintf(log_msg, "%ld [DECRYPTER %d] [INFO] Received new encrypted password: %s", time(NULL), data->id, data->encrypted_password);
                log_message_for_decrypter(data, log_msg);
                
                return true; // New password received
            } else {
                free(new_encrypted);
            }
        }
    } else if (bytes_read < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        // Error reading from broadcast pipe - it might be closed
        close(data->broadcast_fd);
        data->broadcast_fd = -1;
    }
    
    return false; // No new password
}