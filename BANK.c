/**
 **********************************************************
 * @file    BANK.c
 * @author  Spencer Thacker <thackers@msoe.edu>
 * @date    12/10/2024
 * @brief   Provides the Bank server for client ATM's to cennect
 **********************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define INITIAL_ACCOUNTS_CAPACITY 10

// Account structure to hold user data
typedef struct {
    char name[50]; // Account holder's name
    char password[50]; // Account holder's password
    double balance; // Account balance
    pid_t last_used_pid; // PID of the last process that used this account 
    // Sending and saving the PID is legacy code, as it was used to link a client to their bank account, but if multiple accounts signed in on the
    // same ATM, it caused issues when finding the correct account. Now the server returns the index of the account instead of saving the user PID
} Account;

// Global variables
static Account *accounts; // Pointer to the array of accounts
static int accounts_capacity = INITIAL_ACCOUNTS_CAPACITY; // Current capacity of accounts array
static int accounts_count = 0; // Number of accounts currently in use
static pthread_mutex_t accounts_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex to ensure thread safety during account access

// Resize the accounts array when more capacity is needed
void resize_accounts_array();
// Save the current account data to a CSV file
void save_to_csv();
// Load account data from a CSV file
void load_from_csv();
// Handle client interactions (sign-in, create, deposit, withdraw)
void handle_client(int client_socket);
// Handle server shutdown signal (SIGINT)
void handle_signal(int sig);

int main() {
    signal(SIGINT, handle_signal); // Register signal handler for SIGINT (Ctrl+C)

    // Allocate memory for accounts array
    accounts = malloc(accounts_capacity * sizeof(Account));
    if (!accounts) {
        perror("malloc failed"); // If malloc fails, print error and exit
        return EXIT_FAILURE;
    }

    load_from_csv(); // Load account data from CSV file

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create server socket (TCP sockets)
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET; // Address family
    address.sin_addr.s_addr = INADDR_ANY; // Accept connections from any IP address
    address.sin_port = htons(PORT); // Set port number

    // Bind socket to the address and port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Start listening for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Bank server is running and waiting for connections...\n");

    // Accept and handle client connections in a loop
    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }

        pthread_t thread_id;
        // Create a new thread to handle each client request
        pthread_create(&thread_id, NULL, (void *)handle_client, (void *)(intptr_t)new_socket);
        pthread_detach(thread_id);  // Detach the thread to allow automatic cleanup
    }

    return 0;
}

// Resize the accounts array when more capacity is needed
void resize_accounts_array() {
    int new_capacity = accounts_capacity * 2;
    // Reallocate memory for accounts array with double the current capacity
    Account *new_accounts = realloc(accounts, new_capacity * sizeof(Account));

    if (!new_accounts) {
        perror("Failed to resize accounts array"); // If realloc fails, print error
        exit(EXIT_FAILURE);
    }

    accounts = new_accounts; // Update the global accounts pointer
    accounts_capacity = new_capacity; // Update the capacity
}

// Save the current account data to a CSV file
void save_to_csv() {
    FILE *file = fopen("data.csv", "w");
    if (!file) {
        perror("Failed to open data.csv for writing"); // If file opening fails, print error
        return;
    }
    // Loop through all accounts and save them in CSV format
    for (int i = 0; i < accounts_count; i++) {
        fprintf(file, "%s,%s,%.2lf,%d\n", accounts[i].name, accounts[i].password, accounts[i].balance, accounts[i].last_used_pid);
    }
    fclose(file); // Close the file after writing
}

// Load account data from a CSV file
void load_from_csv() {
    FILE *file = fopen("data.csv", "r");
    if (!file) { // If file doesn't exist, nothing to load
        return; 
    }   

    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), file)) {
        if (accounts_count == accounts_capacity) {
            resize_accounts_array();  // Resize accounts array if it is full
        }
        Account account;
        // Parse the CSV line into an account structure
        sscanf(line, "%49[^,],%49[^,],%lf,%d", account.name, account.password, &account.balance, &account.last_used_pid);
        accounts[accounts_count++] = account;  // Add the new account to the array
    }
    fclose(file); // Close the file after reading
}

// Handle client interactions (sign-in, create, deposit, withdraw)
void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];

    while (1) {
        memset(buffer, 0, sizeof(buffer)); // Clear the buffer for new input
        int bytes_read = recv(client_socket, buffer, sizeof(buffer), 0); // Receive the command from the client
        if (bytes_read <= 0) { // If no data is received or connection closed, exit the function
            close(client_socket);
            return;
        }

        char response[BUFFER_SIZE] = {0}; // Prepare the response buffer to send back to the client

        // Handle SIGNIN command
        if (strncmp(buffer, "SIGNIN", 6) == 0) {
            char name[50], password[50];
            sscanf(buffer + 7, "%s %s", name, password); // Extract username and password from the command

            pthread_mutex_lock(&accounts_mutex); // Lock the mutex to ensure thread-safe access to accounts
            sleep(5); // Simulate processing delay
            int found = -1;
            // Search for the account with matching name and password
            for (int i = 0; i < accounts_count; i++) {
                if (strcmp(accounts[i].name, name) == 0 && strcmp(accounts[i].password, password) == 0) {
                    found = i; // If found, store the account index
                    break;
                }
            }
            
            if (found != -1) {
                snprintf(response, sizeof(response), "SUCCESS %d %.2lf", found, accounts[found].balance);
                printf("Sign-in successful for %s. Current balance: %.2lf\n", name, accounts[found].balance);
            } else {
                snprintf(response, sizeof(response), "ERROR Account not found or incorrect password");
                printf("Sign-in failed for %s\n", name);
            }
            pthread_mutex_unlock(&accounts_mutex); // Unlock the mutex after account access
        }
        // Handle CREATE command to create a new account
        else if (strncmp(buffer, "CREATE", 6) == 0) {
            char name[50], password[50];
            sscanf(buffer + 7, "%s %s", name, password); // Extract name and password

            
            pthread_mutex_lock(&accounts_mutex); // Lock the mutex to ensure thread-safe access to accounts
            sleep(5); // Simulate processing delay
            if (accounts_count == accounts_capacity) {
                pthread_mutex_unlock(&accounts_mutex); // Unlock before resizing
                resize_accounts_array(); // Resize the array if full
                pthread_mutex_lock(&accounts_mutex); // Re-lock after resizing
            }

            // Create a new account
            strcpy(accounts[accounts_count].name, name);
            strcpy(accounts[accounts_count].password, password);
            accounts[accounts_count].balance = 0.0;
            accounts[accounts_count].last_used_pid = getpid();
            
            printf("New account created: Name = %s, Index = %d\n", name, accounts_count);
            accounts_count++;
            pthread_mutex_unlock(&accounts_mutex);

            snprintf(response, sizeof(response), "SUCCESS");
        }
        // Handle DEPOSIT command
        else if (strncmp(buffer, "DEPOSIT", 7) == 0) {
            int index;
            double amount;
            sscanf(buffer + 8, "%d %lf", &index, &amount);  // Extract account index and deposit amount

            pthread_mutex_lock(&accounts_mutex); // Lock the mutex to ensure thread-safe access to accounts
            sleep(5); // Simulate processing delay
            if (index >= 0 && index < accounts_count) { // Check if the account index is valid
                accounts[index].balance += amount; // Deposit amount to the account
                snprintf(response, sizeof(response), "SUCCESS %.2lf", accounts[index].balance);
                printf("Deposit: Account %d, Amount %.2lf, New Balance %.2lf\n", index, amount, accounts[index].balance);
            } else {
                snprintf(response, sizeof(response), "ERROR Invalid account index");
                printf("Deposit failed: Invalid account index %d\n", index);
            }
            pthread_mutex_unlock(&accounts_mutex); // Unlock the mutex after account access
        }
        // Handle WITHDRAW command
        else if (strncmp(buffer, "WITHDRAW", 8) == 0) {
            int index;
            double amount;
            sscanf(buffer + 9, "%d %lf", &index, &amount);  // Extract account index and withdraw amount

            pthread_mutex_lock(&accounts_mutex); // Lock the mutex to ensure thread-safe access to accounts
            sleep(5); // Simulate processing delay
            if (index >= 0 && index < accounts_count) { // Check if the account index is valid
                if (accounts[index].balance >= amount) { // Check if the balance is sufficient for withdrawal
                    accounts[index].balance -= amount; // Withdraw the amount
                    snprintf(response, sizeof(response), "SUCCESS %.2lf", accounts[index].balance);
                    printf("Withdrawal: Account %d, Amount %.2lf, New Balance %.2lf\n", index, amount, accounts[index].balance);
                } else {
                    snprintf(response, sizeof(response), "ERROR Insufficient funds");
                    printf("Withdrawal failed: Insufficient funds for Account %d\n", index);
                }
            } else {
                snprintf(response, sizeof(response), "ERROR Invalid account index");
                printf("Withdrawal failed: Invalid account index %d\n", index);
            }
            pthread_mutex_unlock(&accounts_mutex); // Unlock the mutex after account access
        } else {
            snprintf(response, sizeof(response), "ERROR Unknown command");
        }

        send(client_socket, response, strlen(response), 0);  // Send the response back to the client
    }
}

// Handle server shutdown signal (SIGINT)
void handle_signal(int sig) {
    printf("\nShutting down server and saving data to CSV...\n");
    save_to_csv(); // Save account data to CSV before shutting down
    free(accounts); // Free dynamic memory
    exit(0);
}
