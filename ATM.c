/**
 **********************************************************
 * @file    ATM.c
 * @author  Spencer Thacker <thackers@msoe.edu>
 * @date    12/10/2024
 * @brief   Provides the client interface to connect to the bank
 **********************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    // Initialize socket variable and server address structure
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    int account_index = -1; // To store account index after sign-in

    // Create socket and handle error if failed
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error\n");
        return -1;
    }

    // Set server address and port
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IP address to binary form and handle invalid address
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("Invalid address or address not supported\n");
        return -1;
    }

    // Attempt to connect to server and handle failure
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connection failed\n");
        return -1;
    }

    while (1) {
        // Display client menu and options
        printf("Select an option:\n");
        printf("1. Sign in\n");
        printf("2. Create account\n");
        if (account_index != -1) { // If signed in, show options for deposit and withdraw
            printf("3. Deposit\n");
            printf("4. Withdraw\n");
        }
        printf("q. Quit\n");

        // Read user choice
        char choice;
        scanf(" %c", &choice);

        // Handle client exit
        if (choice == 'q') {
            printf("Exiting client.\n");
            close(sock);  // Close socket before exiting
            return 0;
        }

        // Sign-in process (choice == '1')
        if (choice == '1') {
            char name[50], password[50];
            printf("Enter name: ");
            scanf("%s", name);  // Read user input for name
            printf("Enter password: ");
            scanf("%s", password);  // Read user input for password

            // Send sign-in request to server
            snprintf(buffer, sizeof(buffer), "SIGNIN %s %s", name, password);
            send(sock, buffer, strlen(buffer), 0);
            memset(buffer, 0, sizeof(buffer));  // Clear buffer
            recv(sock, buffer, sizeof(buffer), 0);  // Receive server response

            // Check if sign-in was successful
            if (strncmp(buffer, "SUCCESS", 7) == 0) {
                double balance;
                sscanf(buffer + 8, "%d %lf", &account_index, &balance);  // Parse account index and balance
                printf("Sign in successful. Account index: %d, Balance: %.2lf\n", account_index, balance);
            } else {
                printf("Sign in failed: %s\n", buffer); // Display error message if sign-in fails
            }
        } 
        // Account creation process (choice == '2')
        else if (choice == '2') {
            char name[50], password[50];
            printf("Enter new account name: ");
            scanf("%s", name);  // Read user input for new account name
            printf("Enter new password: ");
            scanf("%s", password);  // Read user input for new password

            // Send account creation request to server
            snprintf(buffer, sizeof(buffer), "CREATE %s %s", name, password);
            send(sock, buffer, strlen(buffer), 0);
            memset(buffer, 0, sizeof(buffer));  // Clear buffer
            recv(sock, buffer, sizeof(buffer), 0);  // Receive server response

            // Check if account creation was successful
            if (strncmp(buffer, "SUCCESS", 7) == 0) {
                printf("Account created successfully.\n");
            } else {
                printf("Account creation failed: %s\n", buffer); // Display error message if creation fails
            }

        } 
        // Deposit process (choice == '3' and account signed in)
        else if (choice == '3' && account_index != -1) {
            double amount;
            printf("Enter deposit amount: ");
            scanf("%lf", &amount); // Read deposit amount

            // Send deposit request to server
            snprintf(buffer, sizeof(buffer), "DEPOSIT %d %lf", account_index, amount);
            send(sock, buffer, strlen(buffer), 0);
            memset(buffer, 0, sizeof(buffer)); // Clear buffer
            recv(sock, buffer, sizeof(buffer), 0); // Receive server response

            // Check if deposit was successful
            if (strncmp(buffer, "SUCCESS", 7) == 0) {
                double balance;
                sscanf(buffer + 8, "%lf", &balance); // Parse new balance
                printf("Deposit successful. New balance: %.2lf\n", balance);
            } else {
                printf("Deposit failed: %s\n", buffer); // Display error message if deposit fails
            }

        } 
        // Withdrawal process (choice == '4' and account signed in)
        else if (choice == '4' && account_index != -1) {
            double amount;
            printf("Enter withdrawal amount: ");
            scanf("%lf", &amount); // Read withdrawal amount

            // Send withdrawal request to server
            snprintf(buffer, sizeof(buffer), "WITHDRAW %d %lf", account_index, amount);
            send(sock, buffer, strlen(buffer), 0);
            memset(buffer, 0, sizeof(buffer)); // Clear buffer
            recv(sock, buffer, sizeof(buffer), 0); // Receive server response

            // Check if withdrawal was successful
            if (strncmp(buffer, "SUCCESS", 7) == 0) {
                double balance;
                sscanf(buffer + 8, "%lf", &balance); // Parse new balance
                printf("Withdrawal successful. New balance: %.2lf\n", balance);
            } else {
                printf("Withdrawal failed: %s\n", buffer); // Display error message if withdrawal fails
            }

        } 
        // Handle invalid choice or unavailable operation
        else {
            printf("Invalid choice or operation unavailable.\n");
        }
    }

    close(sock); // Close socket before exiting
    return 0;
}
