
# EventListening() Manual Page

## ABSTRACT

Handles incoming events through TCP connections and manages the transfer of data.

## LIBRARY

LibNaegCommon

## SYNOPSIS

```cpp
#include "EventListening.h"

// creation
static EventListening *create();

// start / cancel
Processing *start(Processing *pChild, DriverMode driver = DrivenByParent);
Processing *cancel(Processing *pChild);

// success
Success success();

// result
static ssize_t pop(const std::string &refMsg, Json::Value &msgEvent);

// repel
Processing *repel(Processing *pChild);
Processing *whenFinishedRepel(Processing *pChild);
```

## DESCRIPTION

The **EventListening()** class manages the listening for events on TCP connections. It can accept new connections, receive data, and process incoming messages. It is designed for handling event-driven programming models in networking applications.

## CREATION

### `static EventListening *create()`

Creates a new instance of the **EventListening()** class. Memory is allocated using `new` with the `std::nothrow` modifier to ensure safe handling of failed allocations.

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

As long as this process is not finished, its function **success()** returns **Pending**.
On error, success() is **not Positive** but returns some negative number.
On success, success() returns **Positive**.

## RESULT

### `static ssize_t pop(const std::string &refMsg, Json::Value &msgEvent)`

Retrieves and processes an event message.

- **refMsg**: A reference message string used to identify the event.
- **msgEvent**: A JSON object that will be populated with event data.

## REPEL

### `Processing *repel(Processing *pChild)`

After a process has completed and its results have been consumed, the process must be separated from the parent process using the **repel()** function. This is inherent to the **nature of processes**.

## EXAMPLES

### Example: Basic Event Listening

In the header file of **MsgDispatching()**
```cpp
/* member variables */
EventListening *mpEventListening;
```

In the source file of **MsgDispatching()**
```cpp
MsgDispatching::MsgDispatching()
  : Processing("MsgDispatching")
  , mpEventListening(NULL) // initialize pointer
{
  mState = StStart;
}

Success MsgDispatching::process()
{
  Success success;
  Value reply;
  string answer;

  switch (mState)
  {
  case StStart:

    // create AND CHECK
    mpEventListening = EventListening::create();
    if (!mpEventListening)
      return procErrLog(-1, "could not create process");

    // start listening for events
    start(mpEventListening);

    mState = StMain;

    break;
  case StMain:

    if (EventListening::pop("command-system", reply) < 1)
      break;

    // process events
    answer = reply["answer"].asString();

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
- Windows
- FreeBSD
- MacOSX

## RECURSION

```
Order                 1
Depth                 -
```

## NOTES

**EventListening()** can be used as a service process. This means that the lifespan of this process can be the same as that of the parent.

## DEPENDENCIES

### SystemCore

The base structure for all software systems.

```
License               MIT
Required              Yes
Project Page          https://github.com/fractal-programming
Documentation         https://github.com/fractal-programming/SystemCore
Sources               https://github.com/fractal-programming/SystemCore
```

### jsoncpp

This C++ library designed for parsing, generating, and manipulating JSON (JavaScript Object Notation) data. It provides a simple and efficient API for working with JSON structures, allowing developers to easily read and write JSON files or strings. The library supports various JSON data types, including objects, arrays, strings, numbers, booleans, and null values. 

With features like easy serialization and deserialization, along with support for both UTF-8 and UTF-16 encodings, libjsoncpp is widely used in applications that require seamless integration with JSON data, such as web applications and configuration management systems. The library is open-source and can be easily integrated into various C++ projects.

```
License               MIT
Required              Yes
Project Page          github.com/open-source-parsers/
Documentation         en.wikibooks.org/wiki/JsonCpp
Sources               github.com/open-source-parsers/
```

## SEE ALSO

**Processing()**, **TcpListening()**, **TcpTransfering()**, **libjsoncpp**

## COPYRIGHT

Copyright (C) 2024, Johannes Natter

## LICENSE

This program is distributed under the terms of the GNU General Public License v3 or later. See <http://www.gnu.org/licenses/> for more information.

---

