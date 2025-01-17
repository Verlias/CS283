#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#define BUFFER_SZ 50

//prototypes
void usage(char *);
void print_buff(char *, int);
int  setup_buff(char *, char *, int);

//prototypes for functions to handle required functionality
int  count_words(char *, int, int);
//add additional prototypes here


int setup_buff(char *buff, char *user_str, int len){
    int user_str_len = 0; 
    char *src = user_str; 
    char *dst = buff;    
    int last_non_space = -1;

    while (*src != '\0') { 
        if (*src != ' ' && *src != '\t') {
            if (user_str_len >= len) {
                return -1;  
            }
            *dst++ = *src;
            user_str_len++;
            last_non_space = user_str_len;
        } else if (user_str_len > 0 && *(dst-1) != ' ') {
            if (user_str_len >= len) {
                return -1;  
            }
            *dst++ = ' ';
            user_str_len++;
        }
        src++;
    }

    if (last_non_space != -1 && last_non_space < user_str_len) {
        user_str_len = last_non_space;
        dst = buff + user_str_len;
    }

    // Fill the rest with dots
    while (user_str_len < len) {
        *dst++ = '.';
        user_str_len++;
    }

    return user_str_len;
}


    

void print_buff(char *buff, int len){
    printf("Buffer:  [");
    for (int i=0; i<len; i++){
        putchar(*(buff+i));
    }
    printf("]\n");
}

void usage(char *exename){
    printf("usage: %s [-h|c|r|w|x] \"string\" [other args]\n", exename);

}

int count_words(char *buff, int len, int str_len) {
    int count = 0;
    int in_word = 0;

    for (int i = 0; i < str_len; i++) {
        if (*(buff + i) != ' ' && *(buff + i) != '.') { // Only check for spaces and dots
            if (!in_word) {
                count++; 
                in_word = 1; 
            }
        } else {
            in_word = 0; 
        }
    }
    return count; 
}

//ADD OTHER HELPER FUNCTIONS HERE FOR OTHER REQUIRED PROGRAM OPTIONS

void reverse_string(char *buff, int str_len) {
    char temp;
    int start = 0;
    int end = str_len - 1;

    // Find the last non-dot character
    while (end >= 0 && buff[end] == '.') {
        end--;
    }

    // Reverse the string in place
    while (start < end) {
        temp = buff[start];
        buff[start] = buff[end];
        buff[end] = temp;
        start++;
        end--;
    }
}


void print_words(char *buff, int len, int str_len) {
    printf("Word Print\n");
    printf("----------\n");
    
    int word_count = 0;
    char *word_start = buff;
    
    for (int i = 0; i < str_len; i++) {
        if (*(buff + i) == ' ' || *(buff + i) == '.') {
            if (word_start != (buff + i)) {
                printf("%d. %.*s(%d)\n", ++word_count, (int)(buff + i - word_start), word_start, (int)(buff + i - word_start));
            }
            word_start = buff + i + 1;
        }
    }

    if (word_start < buff + str_len) {
        printf("%d. %s(%d)\n", ++word_count, word_start, (int)(buff + str_len - word_start));
    }

    printf("\nNumber of words returned: %d\n", word_count);
}



int main(int argc, char *argv[]){

    char *buff;             //placehoder for the internal buffer
    char *input_string;     //holds the string provided by the user on cmd line
    char opt;               //used to capture user option from cmd line
    int  rc;                //used for return codes
    int  user_str_len;      //length of user supplied string

    //TODO:  #1. WHY IS THIS SAFE, aka what if arv[1] does not exist?
    //      This is safe because the way that the conditional is that it first
    //      evaluates argc < 2 -- This means that if the first conditional is true
    //      then the second conditional will never be evaluated as long as argc < 2 is True
    //      So if the first conditional is false then we know argv is safe to eval. 
    //      Because we know there is at least two arguments -- this argv[1] being able to execute

    if ((argc < 2) || (*argv[1] != '-')){
        usage(argv[0]);
        exit(1);
    }

    opt = (char)*(argv[1]+1);   //get the option flag

    //handle the help flag and then exit normally
    if (opt == 'h'){
        usage(argv[0]);
        exit(0);
    }

    //WE NOW WILL HANDLE THE REQUIRED OPERATIONS

    //TODO:  #2 Document the purpose of the if statement below
    //      The purpose of argc < 3 conditional is to check if the user is inputting the
    //      correct amount of arguments. If the user is inputting less than 3 then they are
    //      prompted the correct useage on how to use the program. If they have more than 3 
    //      then they are able to proceed without being told how to use the program.
    //      By doing this if statement this ensure that later on in the program
    //      we dont access things like argc[2] when it doesnt exist
    if (argc < 3){
        usage(argv[0]);
        exit(1);
    }

    input_string = argv[2]; //capture the user input string

    //TODO:  #3 Allocate space for the buffer using malloc and
    //          handle error if malloc fails by exiting with a 
    //          return code of 99
    // CODE GOES HERE FOR #3
    buff = (char *)malloc(BUFFER_SZ * sizeof(char));
    if (buff == NULL) {
        printf("Error: Memory Allocation Failed.\n");
        exit(99);
    }






    user_str_len = setup_buff(buff, input_string, BUFFER_SZ);     //see todos
    if (user_str_len < 0){
        printf("Error setting up buffer, error = %d", user_str_len);
        exit(2);
    }

    switch (opt){
        case 'c':
            rc = count_words(buff, BUFFER_SZ, user_str_len);  //you need to implement
            if (rc < 0){
                printf("Error counting words, rc = %d", rc);
                exit(2);
            }
            printf("Word Count: %d\n", rc);
            break;

        //TODO:  #5 Implement the other cases for 'r' and 'w' by extending
        //       the case statement options

        case 'r':
            reverse_string(buff, user_str_len); 
            break;

        case 'w':
            print_words(buff, BUFFER_SZ, user_str_len);
            break;

        case 'x':
            if (argc != 5) { 
                usage(argv[0]);
                exit(1);
            }
            printf("Not Implemented!\n");
            exit(3); 


                default:
                    usage(argv[0]);
                    exit(1);
            }

        //TODO:  #6 Dont forget to free your buffer before exiting
        print_buff(buff,BUFFER_SZ);
        free(buff);
        exit(0);
}

//TODO:  #7  Notice all of the helper functions provided in the 
//          starter take both the buffer as well as the length.  Why
//          do you think providing both the pointer and the length
//          is a good practice, after all we know from main() that 
//          the buff variable will have exactly 50 bytes?
//          
//          I think it is good practice to take both buffer as well as the length
//          because it allows the programmer to have more flexibility with handling of different
//          buffer sizes in future development. Overall, It makes coding easier as it will allow
//          the developer to know whats happening in terms of memory and whats happening.

// Although we know that buff will always have exactly 50 bytes, 
// this approach allows for easy adjustments if the buffer size 
// changes or if different sizes are used elsewhere in the program.
// Additionally, explicitly passing the length helps prevent bugs like 
// buffer overflows by clearly indicating how much of the buffer is valid for processing.