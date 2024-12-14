# System Programming Lab 13 Multiprocessing
## Spencer Thacker <thackers@msoe.edu>

This program aims to recreate Producer/consumer problems in the form of a bank account using sockets where multiple clients can send signals to the bank at once, with each being added to a queue if a process is already happening.

## Server (BANK.c)
On startup, BAMK.c loads from a .CSV file named data.csv into a dynamic array of structs using malloc that increases in size when full. Each struct holds the name, password, balance and the last used PID of the client. If the file is not found, the program will continue as normal.


Next, the Bank waits for client inputs from an ATM, when it does receive input, it makes a thread to handle the input and then detaches its thread, now in the pointer function handle_client uses a mutex to lock other threads from continuing. This causes all other incoming inputs to be added to a queue so it can handle several transactions accurately from multiple sources.


Clients can send the following signal.
1. Account sign-in, where a client sends its account name and login and compares the name and password to ones saved in the structs. If it exists,  it returns its index to its client to be saved and sent in future transactions. It also returns a confirmation message with the current balance in the account. If it cannot find an account with the name and login, it sends an error message back to the client.
2. Account creation, where a client sends a new account name and login, it then adds a new account to the struct, defaulting the current balance to zero and setting the “last used pid” in the struct to the current PID.
3. Deposit, when a client sends the command it also sends the index found in the sign-in. It then prompts the client for the amount to deposit, and that value will be stored in the struct. After it returns a new balance. If the index is not found, return an error message to the client.
4. Withdrawal, when a client sends the command it also sends the index found in the sign-in. If a struct of that index exists, the server will prompt the client for the amount to withdraw, it can not exceed the balance in the back. It then sends the amount withdrawn and the new balance of the account. If the index is not found, return an error message to the client.


Recent signals and their information are printed to the server terminal.
After a request is processed and the server's response is returned, the server unlocks the mutex and begins the next client request in the queue.


On shut down of the server from a control C signal, the saved information in each struct is exported to a .CSV file named data. It is formatted in a manner so it can be read at the startup of the program.


## Client (ATM.c)
On startup, the ATM connects to the server, which then prompts the client for sign-in information (signal 1), or if they want to create an account (signal 2). After confirmation that a sign-in action happened successfully, they can perform an action Deposit (signal 3) or Withdrawal (signal 4). The client can also give a signal to close the server 
If the client types in “q” the client program closes.
