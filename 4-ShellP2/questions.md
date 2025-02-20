# **Shell Implementation & Linux Signals - Q&A**

## **1. Why do we use `fork/execvp` instead of just `execvp`? What’s the benefit of `fork`?**  
> **Answer**: `fork` creates a new child process that runs the command, while the parent (our shell) keeps running. This way, the shell doesn’t get replaced by the new program. It also lets us manage multiple commands, like running stuff in the background or handling multiple processes.  

---

## **2. What happens if `fork()` fails? How does our code handle it?**  
> **Answer**: If `fork()` fails, it returns `-1`, which means the system couldn’t create a new process (probably because of low memory or hitting the process limit). In our implementation, we check for this and print an error with `perror()`, then return an error code so we don’t try to execute an invalid state.  

---

## **3. How does `execvp()` find the command to run? What environment variable is involved?**  
> **Answer**: `execvp()` searches through the directories listed in the `PATH` environment variable. If you type `ls`, it looks in places like `/bin`, `/usr/bin`, etc., until it finds the executable file.  

---

## **4. Why do we call `wait()` in the parent process after forking? What happens if we skip it?**  
> **Answer**: `wait()` makes sure the shell doesn’t move on until the child process finishes. Without `wait()`, the shell could keep running while orphaning zombie processes (which stick around in memory even though they’re dead).  

---

## **5. What does `WEXITSTATUS()` do, and why is it important?**  
> **Answer**: `WEXITSTATUS()` grabs the exit status of the child process after it finishes. This tells us whether the command ran successfully or failed, which is useful if we need to handle errors properly.  

---

## **6. How does `build_cmd_buff()` handle quoted arguments? Why is this important?**  
> **Answer**: It treats everything inside double quotes as a single argument, so `echo "hello world"` stays together instead of being split into `echo`, `hello`, and `world`. This keeps multi-word arguments intact.  

---

## **7. What changed in the parsing logic compared to the last assignment? Any unexpected challenges?**  
> **Answer**: The old parser used `build_cmd_list()`, which split commands using pipes (`|`) and then tokenized each one separately. Now, `build_cmd_buff()` is simpler and doesn’t handle pipes, but we improved argument handling—especially for quoted strings. The tricky part was making sure spaces inside quotes didn’t break the tokenization logic.  

---

# **Linux Signals Research**  

## **8. What are signals in Linux, and how do they compare to other IPC methods?**  
> **Answer**: Signals are like process alerts—they notify a process when something happens, like an interrupt or a termination request. Unlike other IPC methods (pipes, message queues, shared memory) that involve actual data exchange, signals are just one-way notifications.  

---

## **9. Three common signals and their use cases:**  
> - **SIGKILL** (`kill -9`): Instantly stops a process, no questions asked. The process can’t ignore it or clean up before dying.  
> - **SIGTERM** (`kill <pid>`): Politely asks a process to stop, giving it a chance to save data or exit gracefully.  
> - **SIGINT** (`Ctrl+C`): Lets the user interrupt a running process (like stopping a script). The process can choose to handle or ignore it.  

---

## **10. What happens when a process gets SIGSTOP? Can it be ignored like SIGINT?**  
> **Answer**: Nope, `SIGSTOP` is like a hard pause—it completely freezes the process until it’s resumed with `SIGCONT`. Unlike `SIGINT`, a process **can’t** catch or ignore it, which makes sure it actually stops.  

---

Let me know if you need any further tweaks! 🚀
