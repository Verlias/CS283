### **1. Ensuring All Child Processes Complete**
When executing piped commands, the shell forks multiple child processes. To make sure they all finish before accepting new input, we use `waitpid()` in a loop to wait for each child process to terminate. Without `waitpid()`, the shell would keep running without waiting, leading to "zombie processes"—child processes that have finished but still occupy system resources. This could eventually slow down or crash the shell if too many zombie processes pile up.

---

### **2. Why Close Unused Pipe Ends?**
`dup2()` is used to redirect input and output file descriptors, but after redirection, the original pipe ends are no longer needed. Leaving them open can cause weird behavior, like processes hanging indefinitely because they think the pipe is still in use. For example, if the write-end of a pipe isn't closed, the reading process may wait forever for more input. Closing unused pipe ends ensures that processes properly detect when input/output is done.

---

### **3. Why `cd` is a Built-in Command**
Unlike external commands, `cd` changes the shell's working directory, which affects the **current shell process**. If `cd` were implemented as an external command, it would run in a child process, and when that process ends, the working directory of the parent shell wouldn't change. This would make `cd` completely useless! The only way to make `cd` work properly is to implement it as a built-in command that modifies the shell's environment.

---

### **4. Supporting Unlimited Piped Commands**
Right now, the shell has a fixed limit on piped commands (`CMD_MAX`). To remove this limit, we could use **dynamic memory allocation**, such as using a **linked list or dynamically resizing an array** with `realloc()`. The trade-off? More flexibility, but also more complexity in managing memory properly (avoiding leaks and fragmentation). Additionally, having too many piped commands could slow things down, so we'd need to consider a reasonable upper limit to avoid performance issues.
