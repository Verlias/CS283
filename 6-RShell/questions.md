### 1. How does the remote client know when it's gotten all the output for a command from the server? What can be done to handle cases where the data comes in pieces?

The client figures out that the output is complete by waiting for a special end-of-transmission marker from the server—specifically, `RDSH_EOF_CHAR` (0x04). Because TCP sends data in chunks, you might only get part of the response at a time. To deal with this, the client keeps reading using `recv()` and buffers the incoming data until it sees that marker. This way, even if the message comes in bits and pieces, the client can combine them until it has the full response.

---

### 2. Since TCP is a reliable stream that doesn’t keep message boundaries, how should a network shell protocol define and detect where a command starts and ends? What could go wrong if this isn’t handled correctly?

Because TCP just gives you a continuous stream of bytes, the protocol needs to clearly define where each command starts and ends. One way to do this is by appending a null terminator (`\0`) to the end of each command and using `RDSH_EOF_CHAR` to mark the end of the responses. If you don't set clear boundaries, the client might end up with incomplete or merged messages, which can lead to commands being executed incorrectly or the server hanging while waiting for more data.

---

### 3. What’s the difference between stateful and stateless protocols in general?

In a nutshell, stateful protocols remember what happened in previous interactions. They keep session details around between messages—like TCP, which holds the connection open for reliable communication, or HTTPS sessions that use cookies. On the other hand, stateless protocols treat every request as a separate, independent transaction without any memory of past exchanges (think of basic HTTP or UDP). While stateful protocols offer reliability and continuity, they require more resources; stateless ones are quicker and easier to scale but might need extra work to ensure things work reliably.

---

### 4. UDP is known as "unreliable"—if that’s the case, why would anyone use it?

Despite being labeled “unreliable” because it doesn’t guarantee packet delivery, order, or error checking, UDP is still very useful. It’s much faster than TCP because it has much less overhead, which makes it ideal for real-time applications like video calls, live streaming, and online gaming where a few lost packets aren’t a big deal. These applications often have their own mechanisms to handle any lost data, so they prefer UDP’s speed over the reliability of TCP.

---

### 5. What interface does the operating system provide to let applications use network communications?

The operating system offers the **sockets API** for network communications. Sockets work like file handles but are specifically designed for network data. They provide functions such as `socket()`, `bind()`, `listen()`, `accept()`, `connect()`, `send()`, and `recv()`, which make it easier for programs to send and receive data over TCP or UDP without getting bogged down by low-level networking details.

---
