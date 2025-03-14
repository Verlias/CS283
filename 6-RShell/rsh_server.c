
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <fcntl.h>

//INCLUDES for extra credit
//#include <signal.h>
//#include <pthread.h>
//-------------------------

#include "dshlib.h"
#include "rshlib.h"


/*
 * start_server(ifaces, port, is_threaded)
 *      ifaces:  a string in ip address format, indicating the interface
 *              where the server will bind.  In almost all cases it will
 *              be the default "0.0.0.0" which binds to all interfaces.
 *              note the constant RDSH_DEF_SVR_INTFACE in rshlib.h
 * 
 *      port:   The port the server will use.  Note the constant 
 *              RDSH_DEF_PORT which is 1234 in rshlib.h.  If you are using
 *              tux you may need to change this to your own default, or even
 *              better use the command line override -s implemented in dsh_cli.c
 *              For example ./dsh -s 0.0.0.0:5678 where 5678 is the new port  
 * 
 *      is_threded:  Used for extra credit to indicate the server should implement
 *                   per thread connections for clients  
 * 
 *      This function basically runs the server by: 
 *          1. Booting up the server
 *          2. Processing client requests until the client requests the
 *             server to stop by running the `stop-server` command
 *          3. Stopping the server. 
 * 
 *      This function is fully implemented for you and should not require
 *      any changes for basic functionality.  
 * 
 *      IF YOU IMPLEMENT THE MULTI-THREADED SERVER FOR EXTRA CREDIT YOU NEED
 *      TO DO SOMETHING WITH THE is_threaded ARGUMENT HOWEVER.  
 */
int start_server(char *ifaces, int port, int is_threaded) {
    (void)is_threaded;
    int svr_socket = boot_server(ifaces, port);
    if (svr_socket < 0) {
        return svr_socket;
    }

    int rc = process_cli_requests(svr_socket);
    stop_server(svr_socket);

    return rc;
}

/*
 * stop_server(svr_socket)
 *      svr_socket: The socket that was created in the boot_server()
 *                  function. 
 * 
 *      This function simply returns the value of close() when closing
 *      the socket.  
 */
int stop_server(int svr_socket){
    return close(svr_socket);
}

/*
 * boot_server(ifaces, port)
 *      ifaces & port:  see start_server for description.  They are passed
 *                      as is to this function.   
 * 
 *      This function "boots" the rsh server.  It is responsible for all
 *      socket operations prior to accepting client connections.  Specifically: 
 * 
 *      1. Create the server socket using the socket() function. 
 *      2. Calling bind to "bind" the server to the interface and port
 *      3. Calling listen to get the server ready to listen for connections.
 * 
 *      after creating the socket and prior to calling bind you might want to 
 *      include the following code:
 * 
 *      int enable=1;
 *      setsockopt(svr_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
 * 
 *      when doing development you often run into issues where you hold onto
 *      the port and then need to wait for linux to detect this issue and free
 *      the port up.  The code above tells linux to force allowing this process
 *      to use the specified port making your life a lot easier.
 * 
 *  Returns:
 * 
 *      server_socket:  Sockets are just file descriptors, if this function is
 *                      successful, it returns the server socket descriptor, 
 *                      which is just an integer.
 * 
 *      ERR_RDSH_COMMUNICATION:  This error code is returned if the socket(),
 *                               bind(), or listen() call fails. 
 * 
 */
int boot_server(char *ifaces, int port){
    int server_sock;
    struct sockaddr_in server_addr;
    int opt = 1;

    // Create socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return ERR_RDSH_COMMUNICATION;
    }

    // Allow port reuse
    if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(server_sock);
        return ERR_RDSH_COMMUNICATION;
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ifaces);
    server_addr.sin_port = htons(port);

    // Bind socket
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_sock);
        return ERR_RDSH_COMMUNICATION;
    }

    // Start listening
    if (listen(server_sock, 5) < 0) {
        perror("listen");
        close(server_sock);
        return ERR_RDSH_COMMUNICATION;
    }

    printf("Server listening on %s:%d\n", ifaces, port);
    return server_sock;
}

/*
 * process_cli_requests(svr_socket)
 *      svr_socket:  The server socket that was obtained from boot_server()
 *   
 *  This function handles managing client connections.  It does this using
 *  the following logic
 * 
 *      1.  Starts a while(1) loop:
 *  
 *          a. Calls accept() to wait for a client connection. Recall that 
 *             the accept() function returns another socket specifically
 *             bound to a client connection. 
 *          b. Calls exec_client_requests() to handle executing commands
 *             sent by the client. It will use the socket returned from
 *             accept().
 *          c. Loops back to the top (step 2) to accept connecting another
 *             client.  
 * 
 *          note that the exec_client_requests() return code should be
 *          negative if the client requested the server to stop by sending
 *          the `stop-server` command.  If this is the case step 2b breaks
 *          out of the while(1) loop. 
 * 
 *      2.  After we exit the loop, we need to cleanup.  Dont forget to 
 *          free the buffer you allocated in step #1.  Then call stop_server()
 *          to close the server socket. 
 * 
 *  Returns:
 * 
 *      OK_EXIT:  When the client sends the `stop-server` command this function
 *                should return OK_EXIT. 
 * 
 *      ERR_RDSH_COMMUNICATION:  This error code terminates the loop and is
 *                returned from this function in the case of the accept() 
 *                function failing. 
 * 
 *      OTHERS:   See exec_client_requests() for return codes.  Note that positive
 *                values will keep the loop running to accept additional client
 *                connections, and negative values terminate the server. 
 * 
 */
int process_cli_requests(int svr_socket) {
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sock = accept(svr_socket, (struct sockaddr *)&client_addr, &client_len);
        
        if (client_sock < 0) {
            perror("accept");
            return ERR_RDSH_COMMUNICATION;
        }

        printf("Client connected!\n");

        int rc = exec_client_requests(client_sock);
        close(client_sock);

        if (rc == OK_EXIT) {
            printf("Server stopping as requested by client.\n");
            break;
        }
    }
    return OK;
}

/*
 * exec_client_requests(cli_socket)
 *      cli_socket:  The server-side socket that is connected to the client
 *   
 *  This function handles accepting remote client commands. The function will
 *  loop and continue to accept and execute client commands.  There are 2 ways
 *  that this ongoing loop accepting client commands ends:
 * 
 *      1.  When the client executes the `exit` command, this function returns
 *          to process_cli_requests() so that we can accept another client
 *          connection. 
 *      2.  When the client executes the `stop-server` command this function
 *          returns to process_cli_requests() with a return code of OK_EXIT
 *          indicating that the server should stop. 
 * 
 *  Note that this function largely follows the implementation of the
 *  exec_local_cmd_loop() function that you implemented in the last 
 *  shell program deliverable. The main difference is that the command will
 *  arrive over the recv() socket call rather than reading a string from the
 *  keyboard. 
 * 
 *  This function also must send the EOF character after a command is
 *  successfully executed to let the client know that the output from the
 *  command it sent is finished.  Use the send_message_eof() to accomplish 
 *  this. 
 * 
 *  Of final note, this function must allocate a buffer for storage to 
 *  store the data received by the client. For example:
 *     io_buff = malloc(RDSH_COMM_BUFF_SZ);
 *  And since it is allocating storage, it must also properly clean it up
 *  prior to exiting.
 * 
 *  Returns:
 * 
 *      OK:       The client sent the `exit` command.  Get ready to connect
 *                another client. 
 *      OK_EXIT:  The client sent `stop-server` command to terminate the server
 * 
 *      ERR_RDSH_COMMUNICATION:  A catch all for any socket() related send
 *                or receive errors. 
 */
int exec_client_requests(int cli_socket) {
    char buffer[SH_CMD_MAX];
    while (1) {
        ssize_t bytes_received = recv(cli_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            printf("Client disconnected.\n");
            break;
        }

        buffer[bytes_received] = '\0';  // Null-terminate the command
        printf("Executing command: %s\n", buffer);  // Debugging output

        // Handle built-in commands
        if (strcmp(buffer, "stop-server") == 0) {
            close(cli_socket);
            return OK_EXIT;
        } else if (strncmp(buffer, "cd ", 3) == 0) {
            char *dir = buffer + 3;
            if (chdir(dir) == 0) {
                send_message_string(cli_socket, "Directory changed.");
            } else {
                send_message_string(cli_socket, "Failed to change directory.");
            }
            continue;
        }

        command_list_t cmd_list;
        int build_result = build_cmd_list(buffer, &cmd_list);
        if (build_result != OK) {
            send_message_string(cli_socket, "Invalid command.");
            continue;
        }

        // Fork to execute command
        pid_t pid = fork();
        if (pid == 0) {  // Child process
                    // Redirect stdout and stderr to the client socket
                    dup2(cli_socket, STDOUT_FILENO);
                    dup2(cli_socket, STDERR_FILENO);

                    // Close all file descriptors except the client socket
                    for (int fd = 0; fd < getdtablesize(); fd++) {
                        if (fd != cli_socket) {
                            close(fd);
                        }
                    }

                    // Execute the command pipeline
                    execute_pipeline(&cmd_list);

                    // Flush stdout and stderr to ensure all output is sent to the client
                    fflush(stdout);
                    fflush(stderr);

                    exit(0);  // Exit child process after execution
                } 
        else if (pid > 0) {  // Parent process
            waitpid(pid, NULL, 0);
        } 
        else {  // Fork failed
            perror("fork");
            send_message_string(cli_socket, "Failed to execute command.");
        }

        free_cmd_list(&cmd_list);
        send_message_eof(cli_socket);  // Signal end of response
    }
    return OK;
}

/*
 * send_message_eof(cli_socket)
 *      cli_socket:  The server-side socket that is connected to the client

 *  Sends the EOF character to the client to indicate that the server is
 *  finished executing the command that it sent. 
 * 
 *  Returns:
 * 
 *      OK:  The EOF character was sent successfully. 
 * 
 *      ERR_RDSH_COMMUNICATION:  The send() socket call returned an error or if
 *           we were unable to send the EOF character. 
 */
int send_message_eof(int cli_socket) {
    char eof_char = RDSH_EOF_CHAR;
    if (send(cli_socket, &eof_char, sizeof(eof_char), 0) < 0) {
        perror("send EOF");
        return ERR_RDSH_COMMUNICATION;
    }
    return OK;
}


/*
 * send_message_string(cli_socket, char *buff)
 *      cli_socket:  The server-side socket that is connected to the client
 *      buff:        A C string (aka null terminated) of a message we want
 *                   to send to the client. 
 *   
 *  Sends a message to the client.  Note this command executes both a send()
 *  to send the message and a send_message_eof() to send the EOF character to
 *  the client to indicate command execution terminated. 
 * 
 *  Returns:
 * 
 *      OK:  The message in buff followed by the EOF character was 
 *           sent successfully. 
 * 
 *      ERR_RDSH_COMMUNICATION:  The send() socket call returned an error or if
 *           we were unable to send the message followed by the EOF character. 
 */
int send_message_string(int cli_socket, char *buff){
    int len = strlen(buff) + 1; // +1 to include null terminator
    if (send(cli_socket, buff, len, 0) < 0)
    {
        return ERR_RDSH_COMMUNICATION;
    }

    return send_message_eof(cli_socket);
}


/*
 * rsh_execute_pipeline(int cli_sock, command_list_t *clist)
 *      cli_sock:    The server-side socket that is connected to the client
 *      clist:       The command_list_t structure that we implemented in
 *                   the last shell. 
 *   
 *  This function executes the command pipeline.  It should basically be a
 *  replica of the execute_pipeline() function from the last deliverable. 
 *  The only thing different is that you will be using the cli_sock as the
 *  main file descriptor on the first executable in the pipeline for STDIN,
 *  and the cli_sock for the file descriptor for STDOUT, and STDERR for the
 *  last executable in the pipeline.  See picture below:  
 * 
 *      
 *┌───────────┐                                                    ┌───────────┐
 *│ cli_sock  │                                                    │ cli_sock  │
 *└─────┬─────┘                                                    └────▲──▲───┘
 *      │   ┌──────────────┐     ┌──────────────┐     ┌──────────────┐  │  │    
 *      │   │   Process 1  │     │   Process 2  │     │   Process N  │  │  │    
 *      │   │              │     │              │     │              │  │  │    
 *      └───▶stdin   stdout├─┬──▶│stdin   stdout├─┬──▶│stdin   stdout├──┘  │    
 *          │              │ │   │              │ │   │              │     │    
 *          │        stderr├─┘   │        stderr├─┘   │        stderr├─────┘    
 *          └──────────────┘     └──────────────┘     └──────────────┘   
 *                                                      WEXITSTATUS()
 *                                                      of this last
 *                                                      process to get
 *                                                      the return code
 *                                                      for this function       
 * 
 *  Returns:
 * 
 *      EXIT_CODE:  This function returns the exit code of the last command
 *                  executed in the pipeline.  If only one command is executed
 *                  that value is returned.  Remember, use the WEXITSTATUS()
 *                  macro that we discussed during our fork/exec lecture to
 *                  get this value. 
 */
int rsh_execute_pipeline(int cli_sock, command_list_t *clist) {
    int num_cmds = clist->num;
    int num_pipes = num_cmds - 1;
    int pipefd[2 * num_pipes];
    pid_t pids[CMD_MAX];

    // Create the required pipes using a temporary file descriptor array.
    for (int i = 0; i < num_pipes; i++) {
        int temp_pipe[2];
        if (pipe(temp_pipe) < 0) {
            perror("pipe");
            return ERR_EXEC_CMD;
        }
        // Save the read and write ends in our pipefd array.
        pipefd[2 * i] = temp_pipe[0];
        pipefd[2 * i + 1] = temp_pipe[1];
    }

    // Fork a child process for each command in the pipeline.
    for (int i = 0; i < num_cmds; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            return ERR_EXEC_CMD;
        }

        if (pids[i] == 0) {
            // Set up input redirection.
            if (i == 0) {
                // The first command gets input from cli_sock.
                if (dup2(cli_sock, STDIN_FILENO) < 0) {
                    perror("dup2");
                    exit(ERR_EXEC_CMD);
                }
            } else {
                // Subsequent commands read from the previous pipe's read end.
                if (dup2(pipefd[2 * (i - 1)], STDIN_FILENO) < 0) {
                    perror("dup2");
                    exit(ERR_EXEC_CMD);
                }
            }

            // Set up output redirection.
            if (i == num_cmds - 1) {
                // The last command sends output (and errors) to cli_sock.
                if (dup2(cli_sock, STDOUT_FILENO) < 0 || dup2(cli_sock, STDERR_FILENO) < 0) {
                    perror("dup2");
                    exit(ERR_EXEC_CMD);
                }
            } else {
                // Intermediate commands write to the current pipe's write end.
                if (dup2(pipefd[2 * i + 1], STDOUT_FILENO) < 0) {
                    perror("dup2");
                    exit(ERR_EXEC_CMD);
                }
            }

            // Close all pipe file descriptors in the child process.
            for (int j = 0; j < 2 * num_pipes; j++) {
                close(pipefd[j]);
            }

            // Execute the command.
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            perror("execvp");
            exit(ERR_EXEC_CMD);
        }
    }

    // Parent process: close all pipe file descriptors.
    for (int i = 0; i < 2 * num_pipes; i++) {
        close(pipefd[i]);
    }

    // Wait for all child processes and capture the exit status of the last.
    int status;
    for (int i = 0; i < num_cmds; i++) {
        waitpid(pids[i], &status, 0);
    }
    
    return WEXITSTATUS(status);
}


/**************   OPTIONAL STUFF  ***************/
/****
 **** NOTE THAT THE FUNCTIONS BELOW ALIGN TO HOW WE CRAFTED THE SOLUTION
 **** TO SEE IF A COMMAND WAS BUILT IN OR NOT.  YOU CAN USE A DIFFERENT
 **** STRATEGY IF YOU WANT.  IF YOU CHOOSE TO DO SO PLEASE REMOVE THESE
 **** FUNCTIONS AND THE PROTOTYPES FROM rshlib.h
 **** 
 */

/*
 * rsh_match_command(const char *input)
 *      cli_socket:  The string command for a built-in command, e.g., dragon,
 *                   cd, exit-server
 *   
 *  This optional function accepts a command string as input and returns
 *  one of the enumerated values from the BuiltInCmds enum as output. For
 *  example:
 * 
 *      Input             Output
 *      exit              BI_CMD_EXIT
 *      dragon            BI_CMD_DRAGON
 * 
 *  This function is entirely optional to implement if you want to handle
 *  processing built-in commands differently in your implementation. 
 * 
 *  Returns:
 * 
 *      BI_CMD_*:   If the command is built-in returns one of the enumeration
 *                  options, for example "cd" returns BI_CMD_CD
 * 
 *      BI_NOT_BI:  If the command is not "built-in" the BI_NOT_BI value is
 *                  returned. 
 */
Built_In_Cmds rsh_match_command(const char *input) {
    typedef struct {
        const char *cmd;
        Built_In_Cmds value;
    } command_entry;

    command_entry commands[] = {
        { "exit",        BI_CMD_EXIT },
        { "dragon",      BI_CMD_DRAGON },
        { "cd",          BI_CMD_CD },
        { "stop-server", BI_CMD_STOP_SVR },
        { "rc",          BI_CMD_RC }
    };

    size_t n_commands = sizeof(commands) / sizeof(commands[0]);
    for (size_t i = 0; i < n_commands; i++) {
        if (strcmp(input, commands[i].cmd) == 0)
            return commands[i].value;
    }
    return BI_NOT_BI;
}


/*
 * rsh_built_in_cmd(cmd_buff_t *cmd)
 *      cmd:  The cmd_buff_t of the command, remember, this is the 
 *            parsed version fo the command
 *   
 *  This optional function accepts a parsed cmd and then checks to see if
 *  the cmd is built in or not.  It calls rsh_match_command to see if the 
 *  cmd is built in or not.  Note that rsh_match_command returns BI_NOT_BI
 *  if the command is not built in. If the command is built in this function
 *  uses a switch statement to handle execution if appropriate.   
 * 
 *  Again, using this function is entirely optional if you are using a different
 *  strategy to handle built-in commands.  
 * 
 *  Returns:
 * 
 *      BI_NOT_BI:   Indicates that the cmd provided as input is not built
 *                   in so it should be sent to your fork/exec logic
 *      BI_EXECUTED: Indicates that this function handled the direct execution
 *                   of the command and there is nothing else to do, consider
 *                   it executed.  For example the cmd of "cd" gets the value of
 *                   BI_CMD_CD from rsh_match_command().  It then makes the libc
 *                   call to chdir(cmd->argv[1]); and finally returns BI_EXECUTED
 *      BI_CMD_*     Indicates that a built-in command was matched and the caller
 *                   is responsible for executing it.  For example if this function
 *                   returns BI_CMD_STOP_SVR the caller of this function is
 *                   responsible for stopping the server.  If BI_CMD_EXIT is returned
 *                   the caller is responsible for closing the client connection.
 * 
 *   AGAIN - THIS IS TOTALLY OPTIONAL IF YOU HAVE OR WANT TO HANDLE BUILT-IN
 *   COMMANDS DIFFERENTLY. 
 */
Built_In_Cmds rsh_built_in_cmd(cmd_buff_t *cmd) {
    Built_In_Cmds ctype = rsh_match_command(cmd->argv[0]);

    if (ctype == BI_CMD_EXIT) {
        return BI_CMD_EXIT;
    } else if (ctype == BI_CMD_STOP_SVR) {
        return BI_CMD_STOP_SVR;
    } else if (ctype == BI_CMD_RC) {
        return BI_CMD_RC;
    } else if (ctype == BI_CMD_CD) {
        // Change the directory using the second argument.
        chdir(cmd->argv[1]);
        return BI_EXECUTED;
    } else {
        return BI_NOT_BI;
    }
}

