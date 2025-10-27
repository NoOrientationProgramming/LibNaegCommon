
# FileExecuting() Manual Page

## ABSTRACT

Class for executing and managing OS processes.

## LIBRARY

LibNaegCommon

## SYNOPSIS

```cpp
#include "FileExecuting.h"

// creation
static FileExecuting *create();

// configuration

//// common
FileExecuting &msTimeoutSet(uint32_t msTimeout);
FileExecuting &errRedirect();

//// source
FileExecuting &envSet(const VecConstChar &envv, bool dropOld = true);
FileExecuting &sourceEnable();
FileExecuting &sourceSet(const char *pSrc, size_t len = 0, bool autoFree = false);
FileExecuting &sourceSet(const std::string *pStr);
FileExecuting &sourceSet(int fd, bool autoClose = false);
FileExecuting &sourceSet(Transfering *pTrans);

//// commands
FileExecuting &cmdAdd(const VecConstChar &argv);
FileExecuting &cmdAdd(const std::string &cmd, const VecConstChar &argv);
FileExecuting &cmdAdd(const std::string &cmd, int argc, char *argv[]);

FileExecuting &operator|=(const VecConstChar &argv);
FileExecuting &operator|(const VecConstChar &argv);

//// sinks
FileExecuting &sinkEnable(int fdSel = STDOUT_FILENO);
FileExecuting &sinkAdd(char *pDest, size_t len, int fdSel = STDOUT_FILENO);
FileExecuting &sinkAdd(std::string *pStr, int fdSel = STDOUT_FILENO);
FileExecuting &sinkAdd(int fd, bool autoClose = false, int fdSel = STDOUT_FILENO);
FileExecuting &sinkAdd(Transfering *pTrans, int fdSel = STDOUT_FILENO);

// getters
size_t numCommands() const;

// start / cancel
Processing *start(Processing *pChild, DriverMode driver = DrivenByParent);
Processing *cancel(Processing *pChild);

// success
Success success();

// result

//// child control
int sigSend(int sig, ssize_t idx = -1) const;
bool childStopped(ssize_t idx = -1, bool isAnd = false);
bool childTerminated(ssize_t idx = -1, bool isAnd = true);
bool childTerminatedBySig(ssize_t idx = -1, bool isAnd = false);
bool coreDumped(ssize_t idx = -1, bool isAnd = false);
int idChild(ssize_t idx = -1);
int codeSig(ssize_t idx = -1);
int codeRet(ssize_t idx = -1);

//// data exchange
ssize_t send(const void *pData, size_t lenReq);
ssize_t read(void *pBuf, size_t lenReq);
ssize_t sinkRead(void *pBuf, size_t lenReq, int fdSel = STDOUT_FILENO);

// repel
Processing *repel(Processing *pChild);
Processing *whenFinishedRepel(Processing *pChild);
```

## DESCRIPTION

The **FileExecuting()** class allows for executing programms and managing OS processes.
It uses various system calls to handle input/output and to interact with the launched OS processes.

<p align="center">
  <kbd>
    <img src="https://raw.githubusercontent.com/NoOrientationProgramming/LibNaegCommon/refs/heads/main/doc/system/FileExecuting/fe-io.svg" style="width:400px;max-width:100%"/>
  </kbd>
</p>

The **FileExecuting()** process allows for complete and asynchronous control over one or more OS processes. Arguments can be passed, environment variables can be set, and data can be exchanged through stdin, stdout, and stderr. Additionally, signals can be sent to the OS processes, and all states of the OS processes can be provided.

<p align="center">
  <kbd>
    <img src="https://raw.githubusercontent.com/NoOrientationProgramming/LibNaegCommon/refs/heads/main/doc/system/FileExecuting/fe-fork.svg" style="width:530px; max-width:100%;"/>
  </kbd>
  <kbd style="margin-left: 10px;">
    <img src="https://raw.githubusercontent.com/NoOrientationProgramming/LibNaegCommon/refs/heads/main/doc/system/FileExecuting/fe-parent-child-connection.svg" style="width:300px; max-width:100%;"/>
  </kbd>
</p>

After the fork, all necessary connections (pipes) to the OS processes are established, allowing for convenient communication through the **FileExecuting()** process.

## CREATION

### `static FileExecuting *create()`

Creates a new instance of the **FileExecuting()** class.
Memory is allocated using `new` with the `std::nothrow` modifier to ensure safe handling of failed allocations.

## CONFIGURATION

### COMMON

### `FileExecuting &msTimeoutSet(uint32_t msTimeout)`

Sets the timeout for command execution in milliseconds.

- **msTimeout**: Timeout in milliseconds (e.g., 2000).

### `FileExecuting &errRedirect()`

Redirects error outputs of the last launched OS process from stderr to stdout.

### SOURCE

The **FileExecuting()** process allows using one of several internal data sources to send data to a single OS process or a group of OS processes.

<p align="center">
  <kbd>
    <img src="https://raw.githubusercontent.com/NoOrientationProgramming/LibNaegCommon/refs/heads/main/doc/system/FileExecuting/fe-sources.svg" style="width:300px;max-width:100%"/>
  </kbd>
</p>

There are essentially three options. The first (**Closed**) is to not transmit any data, which is the default state. Second, data can be transmitted **manually**. For this, the source must first be enabled using the `sourceEnable()` function. Data can then be sent to the OS processes during runtime with `send()`. The third option is to have the **FileExecuting()** process **automatically** retrieve the data. In automatic mode, sources can be a C-string, a C++ string object, a file descriptor, or a **Transfering()** process.

### `FileExecuting &envSet(const VecConstChar &envv, bool dropOld = true)`

Sets the environment variables for all launched OS processes.

- **envv**: List (vector) of const C-strings containing environment variables.
- **dropOld**: When true the current environment variables will be included, otherwise not.

### `FileExecuting &sourceEnable()`

This function informs the **FileExecuting()** process that the input pipe to the first launched OS process must be established in principle.
This is necessary when an automatic source has not been activated using one of the `sourceSet(...)` functions,
but instead, data needs to be sent using the `send()` function at specific points during the runtime of the OS process.

### `FileExecuting &sourceSet(const char *pSrc, size_t len = 0, bool autoFree = false)`

Sets a C-string as the source for stdin of the first launched OS process.

- **pSrc**: Pointer to C-string from which to read.
- **len**: Number of bytes to be read.
- **autoFree**: Free data automatically with stdlib function `free()` after consumption. Use only, if C-string was created on HEAP.

### `FileExecuting &sourceSet(const std::string *pStr)`

Sets a C++ string object as the source for stdin of the first launched OS process.

- **pSrc**: Pointer to C++ string object from which to read.

### `FileExecuting &sourceSet(int fd, bool autoClose = false)`

Sets a file descriptor as the source for stdin of the first launched OS process.

- **fd**: File descriptor from which to read.
- **autoClose**: Close file automatically after consumption.

### `FileExecuting &sourceSet(Transfering *pTrans)`

Sets a **Transfering()** process as the source for stdin of the first launched OS process.

- **pTrans**: Pointer to the **Transfering()** process from which to read.

### COMMANDS

The **FileExecuting()** process allows for executing one or more OS commands and subsequently communicating with their OS processes.
When more than one command is added, the OS processes are chained together.

<p align="center">
  <kbd>
    <img src="https://raw.githubusercontent.com/NoOrientationProgramming/LibNaegCommon/refs/heads/main/doc/system/FileExecuting/fe-pipe.svg" style="width:300px;max-width:100%"/>
  </kbd>
</p>

This means that the output of one OS process is passed as input to the next OS process.
When data is provided to the **FileExecuting()** process, it sends the data to the first OS process in the chain.
When data is received from the **FileExecuting()** process, it is the output of the last OS process.

The method `cmdAdd()` is overloaded to accept different parameter formats.

### `FileExecuting &cmdAdd(const VecConstChar &argv)`

The first argument element (`argv[0]`) is interpreted as the command.

- **argv**: List (vector) of const C-strings.

### `FileExecuting &cmdAdd(const std::string &cmd, const VecConstChar &argv)`

- **cmd**: Command to be executed.
- **argv**: List (vector) of const C-strings.

### `FileExecuting &cmdAdd(const std::string &cmd, int argc, char *argv[])`

- **cmd**: Command to be executed.
- **argc**: Number of arguments to be passed to the command.
- **argv**: List (vector) of arguments as const C-strings.

### `FileExecuting &operator|=(const VecConstChar &argv)`
### `FileExecuting &operator|(const VecConstChar &argv)`

To further simplify the chaining, the commands can also be added in Bash style using OR operators. The first argument element (`argv[0]`) is interpreted as the command.

- **argv**: List (vector) of const C-strings.

Example: 
```
// grep filters the output of cat

*mpExec |= {"cat",  "my_file.txt"};
*mpExec |= {"grep", "bar"};
```

### SINKS

Similar to the data source, the data sinks can also be configured. Again, there are several options. BUT: In contrast to the data source, multiple simultaneous data sinks can now be activated.

<p align="center">
  <kbd>
    <img src="https://raw.githubusercontent.com/NoOrientationProgramming/LibNaegCommon/refs/heads/main/doc/system/FileExecuting/fe-sinks.svg" style="width:400px;max-width:100%"/>
  </kbd>
</p>

The first option is to **discard** the data. This is the default state. Another option is to **manually** receive the data. In this case, the data point must be enabled again, using `sinkEnable()`. If an **automatic** data sink is specified, enabling the sink is unnecessary. The targets can be a C-string, a C++ string object, a file descriptor, or a **Transfering()** process, just like with the data source. The received data from the last launched OS process will then be written to ALL configured data sinks.

### `FileExecuting &sinkEnable(int fdSel = STDOUT_FILENO)`

Similar to function `sourceEnable()`.
This function informs the **FileExecuting()** process that the selected output pipe (stdout or stderr)
to the last launched OS process must be established in principle.
This is necessary when an automatic sink has not been activated using one of the `sinkAdd(...)` functions,
but instead, data needs to be read using the `read()` function at specific points during the runtime of the OS process.

- **fdSel**: File descriptor, representing the pipe which should be activated (e.g., STDOUT_FILENO = default, or STDERR_FILENO).

### `FileExecuting &sinkAdd(char *pDest, size_t len, int fdSel = STDOUT_FILENO)`

Adds a C-string as an additional data sink for the stdout of the last launched OS process.

- **pDest**: Pointer to C-string which should be filled with data.
- **len**: Maximum number of bytes that can be written into the C-string.
- **fdSel**: File descriptor, representing the pipe from which to read (e.g., STDOUT_FILENO = default, or STDERR_FILENO).

### `FileExecuting &sinkAdd(std::string *pStr, int fdSel = STDOUT_FILENO)`

Adds a C++ string object as an additional data sink for the stdout of the last launched OS process.

- **pStr**: Pointer to C++ string object which should be filled with data.
- **fdSel**: File descriptor, representing the pipe from which to read (e.g., STDOUT_FILENO = default, or STDERR_FILENO).

### `FileExecuting &sinkAdd(int fd, bool autoClose = false, int fdSel = STDOUT_FILENO)`

Adds a file descriptor as an additional data sink for the stdout of the last launched OS process.

- **fd**: Descriptor to file which should be filled with data.
- **autoClose**: Close file automatically after consumption.
- **fdSel**: File descriptor, representing the pipe from which to read (e.g., STDOUT_FILENO = default, or STDERR_FILENO).

### `FileExecuting &sinkAdd(Transfering *pTrans, int fdSel = STDOUT_FILENO)`

Adds a **Transfering()** process as an additional data sink for the stdout of the last launched OS process.

- **pTrans**: Pointer to **Transfering()** process which should be filled with data.
- **fdSel**: File descriptor, representing the pipe from which to read (e.g., STDOUT_FILENO = default, or STDERR_FILENO).

## GETTERS

### `size_t numCommands() const`

Returns the number of added commands.

## START

### `Processing *start(Processing *pChild, DriverMode driver = DrivenByParent)`

Once the process is started, it progresses "in the background".
This means that with each system tick, the process is allowed to take a small amount of processing time.
During each tick, the process must account for other processes that are contained within the same driver tree.

The progression can be managed by the parent process itself (DrivenByParent = default) or optionally by a new driver.
When a new driver is used, it creates a new driver tree.
All children within a driver tree share the processing time of the system ticks, unless a new driver tree is created.

A new driver can either be an internal driver, such as a worker thread (DrivenByNewInternalDriver),
or any external driver (DrivenByExternalDriver), like a thread pool or a specialized scheduler.

- **pChild**: Pointer to any process which is derived from **Processing()**.
- **driver**: Type of driver which is responsible for the progress of the new child process. A new thread? --> DrivenByNewInternalDriver

## SUCCESS

### `Success success()`

Processes are related to functions.
They establish a **mapping from input to output**.
For **functions**, the **mathematical signature** is **y = f(x)**.
In the case of processes, however, the mapping cannot happen immediately as with functions;
instead, it takes too much time to wait for completion.
Therefore, the mathematical signature of **processes** is **y = p(x, t)**.

In **software**, processes also differ from functions.
While **functions** are managed by the compiler and the calling procedure (ABI) on the system's **stack**,
**processes** must be managed by the user and reside in the **heap** memory.

As long as this process is not finished, its function `success()` returns **Pending**.
On error, `success()` is **not Positive** but returns some negative number.
On success, `success()` returns **Positive**.

## RESULT

### CHILD CONTROL

### `int sigSend(int sig, ssize_t idx = -1) const`

Sends a signal to the child process.

- **sig**: Posix signal to be sent (e.g., SIGUSR1).
- **idx**: Index of the command in the chain to which the signal will be sent. Default: Signal will be sent to all OS processes in the chain.

### `bool childStopped(ssize_t idx = -1, bool isAnd = false)`

Checks if the child process has stopped.

- **idx**: Index of the command in the chain from which the status will be retrieved. Default: Status of all OS processes combined.
- **isAnd**: When false the function returns true in case at least one OS process has stopped. When true the function returns true only if all OS processes in the chain have stopped. Only useful if idx == -1

### `bool childTerminated(ssize_t idx = -1, bool isAnd = true)`

Checks if the child process has terminated.

- **idx**: Index of the command in the chain from which the status will be retrieved. Default: Status of all OS processes combined.
- **isAnd**: When false the function returns true in case at least one OS process has terminated. When true the function returns true only if all OS processes in the chain have terminated. Only useful if idx == -1

### `bool childTerminatedBySig(ssize_t idx = -1, bool isAnd = false)`

Checks if the child process was terminated by a signal.

- **idx**: Index of the command in the chain from which the status will be retrieved. Default: Status of all OS processes combined.
- **isAnd**: When false the function returns true in case at least one OS process has terminated by a signal. When true the function returns true only if all OS processes in the chain have terminated by a signal. Only useful if idx == -1

### `bool coreDumped(ssize_t idx = -1, bool isAnd = false)`

Checks if a core dump was produced.

- **idx**: Index of the command in the chain from which the status will be retrieved. Default: Status of all OS processes combined.
- **isAnd**: When false the function returns true in case at least one OS process has produced a core dump. When true the function returns true only if all OS processes in the chain have produced a core dmp. Only useful if idx == -1

### `int idChild(ssize_t idx = -1)`

Returns the PID of the child process.

- **idx**: Index of the command in the chain from which the PID will be retrieved. Default: Last process in the chain.

### `int codeSig(ssize_t idx = -1)`

Returns the termination signal code of the child process.

- **idx**: Index of the command in the chain from which the termination signal code will be retrieved. Default: Last process in the chain.

### `int codeRet(ssize_t idx = -1)`

Returns the return code of the child process.

- **idx**: Index of the command in the chain from which the return code will be retrieved. Default: Last process in the chain.

### DATA EXCHANGE

### `ssize_t send(const void *pData, size_t lenReq)`

Sends data to the first launched OS process.

- **pData**: Pointer to data from which to read.
- **len**: Number of bytes to be read.

### `ssize_t read(void *pBuf, size_t lenReq)`

Reads data from stdout of the last launched OS process.

- **pBuf**: Pointer to buffer which should be filled with data.
- **len**: Maximum number of bytes that can be written into the buffer.

### `ssize_t sinkRead(void *pBuf, size_t lenReq, int fdSel = STDOUT_FILENO)`

Reads data from the desired pipe of the last launched OS process.

- **pBuf**: Pointer to buffer which should be filled with data.
- **len**: Maximum number of bytes that can be written into the buffer.
- **fdSel**: File descriptor, representing the pipe from which to read (e.g., STDOUT_FILENO = default, or STDERR_FILENO).

## EXAMPLES

### Example: Simple Execution of a Command

In the header file of **DataConverting()**
```cpp
  /* member variables */
  FileExecuting *mpExec;
  std::string mStrResult;
```

In the source file of **DataConverting()**
```cpp
DataConverting::DataConverting()
  : Processing("DataConverting")
  , mpExec(NULL) // initialize pointer
  , mStrResult("")
{
  mState = StStart;
}

Success DataConverting::process()
{
  Success success;

  switch (mState)
  {
  case StStart:

    // create AND CHECK
    mpExec = FileExecuting::create();
    if (!mpExec)
      return procErrLog(-1, "could not create process");

    // configure
    *mpExec |= {"cat",  "my_file.txt"};
    *mpExec |= {"grep", "bar"};

    mpExec->sinkAdd(&mStrResult);

    // start
    start(mpExec);

    mState = StExecDoneWait;

    break;
  case StExecDoneWait:

    // wait
    success = mpExec->success();
    if (success == Pending)
      break;

    // FIRST: check for errors
    if (success != Positive)
      return procErrLog(-1, "could not finish command execution");

    // consume result
    procInfLog("Return code is: %d", mpExec->codeRet());
    procInfLog("Output data is: %s", mStrResult.c_str());

    // ALWAYS: repel
    repel(mpExec);
    mpExec = NULL;

    mState = StNext;

    break;
  case StNext:

    ...

    break;
  default:
    break;
  }

  return Pending;
}
```

## SCOPE

- Linux
- FreeBSD
- MacOSX

## RECURSION

```
Order                 2
Depth                 1
```

## NOTES

**FileExecuting()** can be used as a service process.
This means that the lifespan of this process can be the same as that of the parent.

## DEPENDENCIES

### SystemCore

The base structure for all software systems.

```
License               MIT
Required              Yes
Project Page          https://github.com/NoOrientationProgramming
Documentation         https://github.com/NoOrientationProgramming/SystemCore
Sources               https://github.com/NoOrientationProgramming/SystemCore
```

## SEE ALSO

**Transfering()**, **pipe(2)**, **fork(2)**, **dup(2)**, **exec(3)**

## COPYRIGHT

Copyright (C) 2024, Johannes Natter

## LICENSE

This program is distributed under the terms of the GNU General Public License v3 or later. See <http://www.gnu.org/licenses/> for more information.

