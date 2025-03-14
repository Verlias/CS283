#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "dshlib.h"

/*
 * init_cmd_buff - Initialize a command buffer structure.
 * @cb: Pointer to the cmd_buff_t structure to be initialized.
 *
 * This version uses an explicit loop to set all argv elements to NULL.
 *
 * Returns: OK on success.
 */
int init_cmd_buff(cmd_buff_t *cb) {
    cb->argc = 0;
    cb->_cmd_buffer = NULL;
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cb->argv[i] = NULL;
    }
    return OK;
}

/*
 * release_cmd_buff - Free allocated memory within a command buffer.
 * @cb: Pointer to the cmd_buff_t structure.
 *
 * Iterates over the argument vector and frees any allocated memory.
 *
 * Returns: OK after deallocation.
 */
int release_cmd_buff(cmd_buff_t *cb) {
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        if (cb->argv[i]) {
            free(cb->argv[i]);
            cb->argv[i] = NULL;
        }
    }
    if (cb->_cmd_buffer) {
        free(cb->_cmd_buffer);
        cb->_cmd_buffer = NULL;
    }
    cb->argc = 0;
    return OK;
}

/*
 * reset_cmd_buff - Clear a command buffer by releasing and reinitializing it.
 * @cb: Pointer to the cmd_buff_t structure.
 *
 * Returns: OK after the buffer has been reset.
 */
int reset_cmd_buff(cmd_buff_t *cb) {
    release_cmd_buff(cb);
    return init_cmd_buff(cb);
}

/*
 * parse_cmd_line - Tokenize a command line string into arguments.
 * @cmd_line: The input string containing the full command.
 * @cb: Pointer to the cmd_buff_t structure to store tokens.
 *
 * This function traverses the input string using a manual pointer-based approach.
 * It handles tokens enclosed in double quotes and tokens separated by whitespace.
 * Each token is duplicated with strdup and stored in the argv array.
 *
 * Returns:
 *   OK if tokens are parsed successfully,
 *   ERR_CMD_ARGS_BAD if a quoted token is not closed,
 *   ERR_MEMORY on memory allocation failure,
 *   WARN_NO_CMDS if no tokens are found,
 *   ERR_CMD_OR_ARGS_TOO_BIG if the token limit is exceeded.
 */
int parse_cmd_line(char *cmd_line, cmd_buff_t *cb) {
    reset_cmd_buff(cb);
    char *p = cmd_line;
    
    // Skip any initial spaces
    while (*p && isspace((unsigned char)*p)) p++;
    
    while (*p) {
        // Skip additional spaces between tokens
        if (isspace((unsigned char)*p)) {
            p++;
            continue;
        }
        
        char *start = NULL;
        if (*p == '"') {
            // Handle quoted token: advance pointer and mark beginning
            p++;
            start = p;
            // Look for the closing quote manually
            while (*p && *p != '"') p++;
            if (*p != '"') {
                fprintf(stderr, "Unbalanced quotes in command line\n");
                return ERR_CMD_ARGS_BAD;
            }
            *p = '\0'; // Terminate the token string
            p++;       // Move past the closing quote
        } else {
            // Normal token: record the start and find the token boundary
            start = p;
            while (*p && !isspace((unsigned char)*p)) p++;
            // Terminate token if not at end-of-string
            if (*p) {
                *p = '\0';
                p++;
            }
        }
        // Ensure token count is within the allowed maximum
        if (cb->argc >= CMD_ARGV_MAX - 1) {
            fprintf(stderr, "Too many arguments provided\n");
            return ERR_CMD_OR_ARGS_TOO_BIG;
        }
        cb->argv[cb->argc] = strdup(start);
        if (!cb->argv[cb->argc]) {
            perror("strdup");
            return ERR_MEMORY;
        }
        cb->argc++;
    }
    
    if (cb->argc == 0) {
        return WARN_NO_CMDS;
    }
    cb->argv[cb->argc] = NULL;
    return OK;
}

/*
 * split_into_cmds - Divide a full command line into a list of piped commands.
 * @cmd_line: The raw input command line.
 * @clist: Pointer to the command_list_t structure to populate.
 *
 * This function uses strtok_r to split the command line by the pipe character.
 * Each segment is trimmed for leading/trailing whitespace and then parsed
 * into a command buffer.
 *
 * Returns:
 *   OK on success,
 *   WARN_NO_CMDS if no valid commands are found,
 *   ERR_TOO_MANY_COMMANDS if the command count exceeds the limit,
 *   ERR_MEMORY on failure to allocate memory.
 */
int split_into_cmds(char *cmd_line, command_list_t *clist) {
    clist->num = 0;
    char *saveptr = NULL;
    char *segment = strtok_r(cmd_line, PIPE_STRING, &saveptr);
    
    while (segment) {
        // Trim leading whitespace
        while (*segment && isspace((unsigned char)*segment)) segment++;
        
        // Create a copy so we can trim trailing spaces
        char *cmd_copy = strdup(segment);
        if (!cmd_copy) {
            return ERR_MEMORY;
        }
        int len = strlen(cmd_copy);
        while (len > 0 && isspace((unsigned char)cmd_copy[len - 1])) {
            cmd_copy[len - 1] = '\0';
            len--;
        }
        
        // Only process non-empty command segments
        if (cmd_copy[0] != '\0') {
            if (clist->num >= CMD_MAX) {
                free(cmd_copy);
                return ERR_TOO_MANY_COMMANDS;
            }
            init_cmd_buff(&clist->commands[clist->num]);
            // Save the trimmed copy for potential debugging
            clist->commands[clist->num]._cmd_buffer = strdup(cmd_copy);
            if (!clist->commands[clist->num]._cmd_buffer) {
                free(cmd_copy);
                return ERR_MEMORY;
            }
            int rc = parse_cmd_line(cmd_copy, &clist->commands[clist->num]);
            free(cmd_copy);
            if (rc != OK) {
                return rc;
            }
            clist->num++;
        } else {
            free(cmd_copy);
        }
        segment = strtok_r(NULL, PIPE_STRING, &saveptr);
    }
    
    if (clist->num == 0) {
        return WARN_NO_CMDS;
    }
    return OK;
}

/*
 * cleanup_cmd_list - Free memory for all command buffers in the list.
 * @clist: Pointer to the command_list_t structure.
 *
 * Iterates through the command list and releases resources allocated for each.
 *
 * Returns: OK after cleanup.
 */
int cleanup_cmd_list(command_list_t *clist) {
    for (int i = 0; i < clist->num; i++) {
        release_cmd_buff(&clist->commands[i]);
    }
    clist->num = 0;
    return OK;
}

/*
 * exec_local_cmd_loop - Main loop to read and execute user commands locally.
 *
 * This function continuously prompts the user for input, processes it, and then
 * executes a pipeline of commands. It handles built-in commands, empty inputs, and
 * error messages in a loop until an exit command is received.
 *
 * Returns: OK when the shell loop terminates normally.
 */
int exec_local_cmd_loop()
{
    char input_line[SH_CMD_MAX + 1];
    command_list_t cmd_list;
    int rc;

    while (true) {
        // Display the prompt
        printf("%s", SH_PROMPT);

        // Read user input; break on EOF
        if (!fgets(input_line, sizeof(input_line), stdin)) {
            printf("\n");
            break;
        }
        // Remove newline from the input string
        input_line[strcspn(input_line, "\n")] = '\0';

        // Exit if user types the exit command
        if (strcmp(input_line, EXIT_CMD) == 0) {
            printf("exiting...\n");
            break;
        }

        // Warn if input is empty
        if (input_line[0] == '\0') {
            printf("%s\n", CMD_WARN_NO_CMD);
            continue;
        }

        // Tokenize the input line into piped commands
        rc = split_into_cmds(input_line, &cmd_list);
        if (rc == WARN_NO_CMDS) {
            printf("%s\n", CMD_WARN_NO_CMD);
            continue;
        } else if (rc == ERR_TOO_MANY_COMMANDS) {
            printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
            continue;
        } else if (rc != OK) {
            printf("%s\n", CMD_ERR_EXECUTE);
            continue;
        }

        // Run the commands in a pipeline
        rc = execute_pipeline(&cmd_list);
        if (rc != OK) {
            printf("%s\n", CMD_ERR_EXECUTE);
        }

        // Free the resources used for this command list
        cleanup_cmd_list(&cmd_list);
    }
    return OK;
}

/*
 * execute_pipeline - Launch and connect a series of piped commands.
 * @clist: Pointer to the command_list_t structure with parsed commands.
 *
 * This function creates pipes for communication between commands, forks a child
 * for each command, sets up redirection via dup2(), and then executes the command.
 * The parent process waits for all children to complete.
 *
 * Returns:
 *   OK if all commands execute successfully,
 *   ERR_EXEC_CMD if an error occurs during the process.
 */
int execute_pipeline(command_list_t *clist)
{
    int num_cmds = clist->num;
    int total_pipes = (num_cmds - 1) * 2;
    int pipefds[total_pipes];
    pid_t pids[CMD_MAX];

    // Create all necessary pipes
    for (int i = 0; i < num_cmds - 1; i++) {
        if (pipe(pipefds + i * 2) < 0) {
            perror("pipe");
            return ERR_EXEC_CMD;
        }
    }

    // Fork a child for each command
    for (int i = 0; i < num_cmds; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            perror("fork");
            return ERR_EXEC_CMD;
        }
        if (pids[i] == 0) {  // Child process
            // Redirect input if not the first command
            if (i > 0) {
                if (dup2(pipefds[(i - 1) * 2], STDIN_FILENO) < 0) {
                    perror("dup2");
                    exit(ERR_EXEC_CMD);
                }
            }
            // Redirect output if not the last command
            if (i < num_cmds - 1) {
                if (dup2(pipefds[i * 2 + 1], STDOUT_FILENO) < 0) {
                    perror("dup2");
                    exit(ERR_EXEC_CMD);
                }
            }
            // Close all pipe file descriptors in the child
            for (int j = 0; j < total_pipes; j++) {
                close(pipefds[j]);
            }
            // Execute the command; if execvp fails, exit with error
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            perror("execvp");
            exit(ERR_EXEC_CMD);
        }
    }
    
    // Parent process: close all pipe file descriptors
    for (int i = 0; i < total_pipes; i++) {
        close(pipefds[i]);
    }

    // Wait for all children to finish
    int status;
    for (int i = 0; i < num_cmds; i++) {
        if (waitpid(pids[i], &status, 0) < 0) {
            perror("waitpid");
            return ERR_EXEC_CMD;
        }
    }
    return WEXITSTATUS(status);
}

/*
 * identify_builtin - Determine if the given command string is a built-in command.
 * @cmd: The command string (usually the first token in a command buffer).
 *
 * Returns a Built_In_Cmds enumeration value that identifies the built-in command.
 */
Built_In_Cmds identify_builtin(const char *cmd) {
    if (!strcmp(cmd, "exit")) {
        return BI_CMD_EXIT;
    }
    if (!strcmp(cmd, "stop-server")) {
        return BI_CMD_STOP_SVR;
    }
    if (!strncmp(cmd, "cd ", 3)) {
        return BI_CMD_CD;
    }
    return BI_NOT_BI;
}

/*
 * run_builtin_cmd - Execute a built-in command based on its type.
 * @cb: Pointer to the command buffer containing the built-in command and its arguments.
 *
 * This function processes the built-in commands:
 *   - "exit" causes the shell to terminate.
 *   - "cd" changes the current working directory.
 *   - "stop-server" is reserved for server termination (handled elsewhere).
 *
 * Returns:
 *   BI_EXECUTED if the built-in command was processed,
 *   BI_NOT_IMPLEMENTED if the command is not recognized.
 */
Built_In_Cmds run_builtin_cmd(cmd_buff_t *cb) {
    Built_In_Cmds type = identify_builtin(cb->argv[0]);
    if (type == BI_CMD_EXIT) {
        exit(0);
    } else if (type == BI_CMD_STOP_SVR) {
        // Server stop command: logic to be implemented in server code.
    } else if (type == BI_CMD_CD) {
        // Attempt to change the directory using the second argument.
        if (chdir(cb->argv[1]) == 0) {
            printf("Directory changed.\n");
        } else {
            perror("chdir");
        }
        return BI_EXECUTED;
    }
    return BI_NOT_IMPLEMENTED;
}

/* 
 * build_cmd_list - Wrapper function for splitting a command line into a list of commands.
 * @cmd_line: The raw command line input.
 * @clist: Pointer to the command_list_t structure to populate.
 *
 * This wrapper calls split_into_cmds to perform the work.
 *
 * Returns: The result of split_into_cmds.
 */
int build_cmd_list(char *cmd_line, command_list_t *clist) {
    return split_into_cmds(cmd_line, clist);
}

/* 
 * free_cmd_list - Wrapper function to free a list of commands.
 * @cmd_lst: Pointer to the command_list_t structure to clean up.
 *
 * This wrapper calls cleanup_cmd_list to free allocated resources.
 *
 * Returns: The result of cleanup_cmd_list.
 */
int free_cmd_list(command_list_t *cmd_lst) {
    return cleanup_cmd_list(cmd_lst);
}
