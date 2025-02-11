#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "dshlib.h"

/*
 *  build_cmd_list
 *    cmd_line:     the command line from the user
 *    clist *:      pointer to clist structure to be populated
 *
 *  This function builds the command_list_t structure passed by the caller
 *  It does this by first splitting the cmd_line into commands by spltting
 *  the string based on any pipe characters '|'.  It then traverses each
 *  command.  For each command (a substring of cmd_line), it then parses
 *  that command by taking the first token as the executable name, and
 *  then the remaining tokens as the arguments.
 *
 *  NOTE your implementation should be able to handle properly removing
 *  leading and trailing spaces!
 *
 *  errors returned:
 *
 *    OK:                      No Error
 *    ERR_TOO_MANY_COMMANDS:   There is a limit of CMD_MAX (see dshlib.h)
 *                             commands.
 *    ERR_CMD_OR_ARGS_TOO_BIG: One of the commands provided by the user
 *                             was larger than allowed, either the
 *                             executable name, or the arg string.
 *
 *  Standard Library Functions You Might Want To Consider Using
 *      memset(), strcmp(), strcpy(), strtok(), strlen(), strchr()
 */
int build_cmd_list(char *cmd_line, command_list_t *clist)
{
    clist->num = 0;

    char *ptr = cmd_line;
    while (isspace((unsigned char)*ptr)) {
        ptr++;
    }
    if (*ptr == '\0') {
        return WARN_NO_CMDS;
    }

    int cmd_count = 0;
    char *save_ptr;
    char *cmd_segment = strtok_r(cmd_line, PIPE_STRING, &save_ptr);
    while (cmd_segment != NULL) {
        while (isspace((unsigned char)*cmd_segment)) {
            cmd_segment++;
        }
        int seg_len = strlen(cmd_segment);
        while (seg_len > 0 && isspace((unsigned char)cmd_segment[seg_len - 1])) {
            cmd_segment[seg_len - 1] = '\0';
            seg_len--;
        }

        if (strlen(cmd_segment) == 0) {
            cmd_segment = strtok_r(NULL, PIPE_STRING, &save_ptr);
            continue;
        }

        if (cmd_count >= CMD_MAX) {
            return ERR_TOO_MANY_COMMANDS;
        }

        char *sub_save_ptr;
        char *token = strtok_r(cmd_segment, " ", &sub_save_ptr);
        if (token == NULL) {
            cmd_segment = strtok_r(NULL, PIPE_STRING, &save_ptr);
            continue;
        }
        strncpy(clist->commands[cmd_count].exe, token, EXE_MAX - 1);
        clist->commands[cmd_count].exe[EXE_MAX - 1] = '\0';

        clist->commands[cmd_count].args[0] = '\0';
        while ((token = strtok_r(NULL, " ", &sub_save_ptr)) != NULL) {
            if (clist->commands[cmd_count].args[0] != '\0') {
                strncat(clist->commands[cmd_count].args, " ", ARG_MAX - strlen(clist->commands[cmd_count].args) - 1);
            }
            strncat(clist->commands[cmd_count].args, token, ARG_MAX - strlen(clist->commands[cmd_count].args) - 1);
        }

        cmd_count++;
        cmd_segment = strtok_r(NULL, PIPE_STRING, &save_ptr);
    }

    if (cmd_count == 0) {
        return WARN_NO_CMDS;
    }

    clist->num = cmd_count;
    return OK;
}