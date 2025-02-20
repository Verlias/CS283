#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "dshlib.h"

// Helper functions for cmd_buff_t management
// Allocate and initialize the command buffer
int alloc_cmd_buff(cmd_buff_t *cmd_buff) {
    cmd_buff->argc = 0;
    cmd_buff->_cmd_buffer = NULL;
    memset(cmd_buff->argv, 0, sizeof(cmd_buff->argv));
    return OK;
}

// Free the memory allocated for the command buffer
int free_cmd_buff(cmd_buff_t *cmd_buff) {
    if (cmd_buff->_cmd_buffer) {
        free(cmd_buff->_cmd_buffer);
        cmd_buff->_cmd_buffer = NULL;
    }
    return OK;
}

// Clear the command buffer by freeing and reallocating it
int clear_cmd_buff(cmd_buff_t *cmd_buff) {
    free_cmd_buff(cmd_buff);
    return alloc_cmd_buff(cmd_buff);
}

/*
 * Splits the command line into tokens.
 * It trims spaces and treats quoted strings as a single token.
 */
int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    clear_cmd_buff(cmd_buff);
    char *p = cmd_line;
    while (*p != '\0') {
        // Skip leading spaces
        while (isspace((unsigned char)*p)) p++;
        if (*p == '\0') break;
        char *token_start = p;
        if (*p == '"') {
            // Handle quoted strings
            p++; // skip opening quote
            token_start = p;
            while (*p != '"' && *p != '\0') p++;
            if (*p == '\0') {
                fprintf(stderr, "Unbalanced quotes in command line\n");
                return ERR_CMD_ARGS_BAD;
            }
            *p = '\0';
            p++;
        } else {
            // Handle unquoted strings
            while (!isspace((unsigned char)*p) && *p != '\0') p++;
            if (*p != '\0') {
                *p = '\0';
                p++;
            }
        }
        // Add token to argv
        if (cmd_buff->argc < CMD_ARGV_MAX - 1) {
            cmd_buff->argv[cmd_buff->argc] = strdup(token_start);
            if (cmd_buff->argv[cmd_buff->argc] == NULL) {
                perror("strdup");
                return ERR_MEMORY;
            }
            cmd_buff->argc++;
        } else {
            fprintf(stderr, "Too many arguments provided\n");
            return ERR_CMD_OR_ARGS_TOO_BIG;
        }
    }
    cmd_buff->argv[cmd_buff->argc] = NULL;
    if (cmd_buff->argc == 0)
        return WARN_NO_CMDS;
    return OK;
}

// Check if the command is a built-in command.
Built_In_Cmds match_command(const char *input) {
    if (strcmp(input, EXIT_CMD) == 0) {
        return BI_CMD_EXIT;
    } else if (strcmp(input, "cd") == 0) {
        return BI_CMD_CD;
    } else if (strcmp(input, "rc") == 0) {
        return BI_RC;
    } else {
        return BI_NOT_BI;
    }
}

// Execute built-in commands.
Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd) {
    Built_In_Cmds bicmd = match_command(cmd->argv[0]);
    switch (bicmd) {
        case BI_CMD_CD:
            if (cmd->argc < 2) {
                return BI_EXECUTED;
            } else {
                if (chdir(cmd->argv[1]) != 0) {
                    perror("chdir");
                    return ERR_EXEC_CMD;  // Ensure error is returned
                }
                return BI_EXECUTED;
            }
        case BI_CMD_EXIT:
            exit(0);
        case BI_RC:
            // Extra credit: rc command placeholder.
            printf("rc not implemented\n");
            return BI_EXECUTED;
        default:
            return BI_NOT_BI;
    }
}

// Execute external commands using fork/execvp.
int exec_cmd(cmd_buff_t *cmd) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return ERR_EXEC_CMD;
    } else if (pid == 0) {
        // Child process: execute the command
        if (execvp(cmd->argv[0], cmd->argv) == -1) {
            perror("execvp");
            exit(ERR_EXEC_CMD);
        }
    } else {
        // Parent process: wait for the child to finish
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            return ERR_EXEC_CMD;
        }
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        } else {
            return ERR_EXEC_CMD;
        }
    }
}

/*
 * Implement your exec_local_cmd_loop function by building a loop that prompts the 
 * user for input.  Use the SH_PROMPT constant from dshlib.h and then
 * use fgets to accept user input.
 * 
 *      while(1){
 *        printf("%s", SH_PROMPT);
 *        if (fgets(cmd_buff, ARG_MAX, stdin) == NULL){
 *           printf("\n");
 *           break;
 *        }
 *        //remove the trailing \n from cmd_buff
 *        cmd_buff[strcspn(cmd_buff,"\n")] = '\0';
 * 
 *        //IMPLEMENT THE REST OF THE REQUIREMENTS
 *      }
 * 
 *   Also, use the constants in the dshlib.h in this code.  
 *      SH_CMD_MAX              maximum buffer size for user input
 *      EXIT_CMD                constant that terminates the dsh program
 *      SH_PROMPT               the shell prompt
 *      OK                      the command was parsed properly
 *      WARN_NO_CMDS            the user command was empty
 *      ERR_TOO_MANY_COMMANDS   too many pipes used
 *      ERR_MEMORY              dynamic memory management failure
 * 
 *   errors returned
 *      OK                     No error
 *      ERR_MEMORY             Dynamic memory management failure
 *      WARN_NO_CMDS           No commands parsed
 *      ERR_TOO_MANY_COMMANDS  too many pipes used
 *   
 *   console messages
 *      CMD_WARN_NO_CMD        print on WARN_NO_CMDS
 *      CMD_ERR_PIPE_LIMIT     print on ERR_TOO_MANY_COMMANDS
 *      CMD_ERR_EXECUTE        print on execution failure of external command
 * 
 *  Standard Library Functions You Might Want To Consider Using (assignment 1+)
 *      malloc(), free(), strlen(), fgets(), strcspn(), printf()
 * 
 *  Standard Library Functions You Might Want To Consider Using (assignment 2+)
 *      fork(), execvp(), exit(), chdir()
 */
int exec_local_cmd_loop()
{
    char input_line[SH_CMD_MAX + 1];
    int rc = 0;
    cmd_buff_t cmd;

    // Allocate command buffer
    alloc_cmd_buff(&cmd);

    while (1) {
        // Display shell prompt
        printf("%s", SH_PROMPT);
        if (fgets(input_line, sizeof(input_line), stdin) == NULL) {
            printf("\n");
            break;
        }
        // Remove trailing newline
        input_line[strcspn(input_line, "\n")] = '\0';

        if (strlen(input_line) == 0) {
            printf("%s", CMD_WARN_NO_CMD);
            continue;
        }

        // Build command buffer from input line
        rc = build_cmd_buff(input_line, &cmd);
        if (rc == WARN_NO_CMDS) {
            printf("%s", CMD_WARN_NO_CMD);
            continue;
        } else if (rc != OK) {
            free_cmd_buff(&cmd);
            continue;
        }

        // Check if the command is built-in
        Built_In_Cmds bicmd = match_command(cmd.argv[0]);
        if (bicmd != BI_NOT_BI) {
            exec_built_in_cmd(&cmd);
        } else {
            // Execute external command
            rc = exec_cmd(&cmd);
            if (rc == ERR_EXEC_CMD) {
                printf("%s\n", "CMD_ERR_EXECUTE");
            }
        }
        // Free command buffer
        free_cmd_buff(&cmd);
    }
    return OK;
}