
### 1. Why is `fgets()` a good choice for this application?  

`fgets()` is great for reading user input in our shell because it prevents buffer overflows by limiting input to a specified size. Unlike `scanf()`, which stops at the first space, `fgets()` reads the entire line, including spaces and special characters, making it more reliable for handling commands. It also allows us to detect EOF, which is useful when running scripts. Plus, it keeps the newline character (if present), making parsing easier.

### 2. Why did we need to use `malloc()` to allocate memory for `cmd_buff` instead of a fixed-size array?  

Using `malloc()` gives us flexibility in memory allocation, which is super important when dealing with unpredictable input sizes. A fixed-size array could either waste memory if it's too big or cause a buffer overflow if it's too small. With `malloc()`, we can allocate exactly what we need and even resize it later with `realloc()`, making our shell more adaptable.

### 3. Why must `build_cmd_list()` trim leading and trailing spaces from each command? What issues might arise if we didn't?  

Trimming spaces is essential because extra spaces can mess up how commands are parsed and executed. If we don’t trim, we might accidentally treat an empty or space-filled string as a valid command, leading to unexpected behavior. Commands might also fail if they have unwanted spaces at the beginning or end. For example, `"  ls  "` should be treated as `"ls"`, but if we don’t trim, the shell might not recognize it correctly.

### 4. Redirection in Linux Shells  

#### Three Redirection Examples and Challenges in Implementation  

1. **Output Redirection (`>` and `>>`)**  
   - Example:  
     ```sh
     ls > output.txt
     ```
   - This sends the output of `ls` to `output.txt`, overwriting the file if it exists. The challenge in implementing this is handling file permissions and ensuring that output doesn’t get lost if redirection fails.  

2. **Input Redirection (`<`)**  
   - Example:  
     ```sh
     sort < input.txt
     ```
   - Instead of typing input manually, this command reads from `input.txt`. The challenge here is checking if the file exists and handling errors gracefully when it doesn’t.  

3. **Error Redirection (`2>`)**  
   - Example:  
     ```sh
     command 2> error.log
     ```
   - This redirects errors to a file instead of displaying them on the screen. The tricky part is distinguishing between normal output and errors, especially if a command produces both.  

#### Redirection vs. Piping  

Redirection and piping both control input and output, but they serve different purposes. Redirection sends output to a file or takes input from a file, while piping (`|`) sends the output of one command directly into another command. For example,  
```sh
ls | grep "file"
