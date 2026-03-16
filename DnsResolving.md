
# DnsResolving() Manual Page

## ABSTRACT

Class for resolving hostnames to IP addresses.

## LIBRARY

LibNaegCommon

## SYNOPSIS

```cpp
#include "DnsResolving.h"

// creation
static DnsResolving *create();

// configuration
void hostnameSet(const std::string &hostname);

// start / cancel
Processing *start(Processing *pChild, DriverMode driver = DrivenByParent);
Processing *cancel(Processing *pChild);

// success
Success success();

// result
const std::list<std::string> &lstIPv4();
const std::list<std::string> &lstIPv6();

// repel
Processing *repel(Processing *pChild);
Processing *whenFinishedRepel(Processing *pChild);
```

## DESCRIPTION

The **DnsResolving()** class allows for the resolution of hostnames into IPv4 and IPv6 addresses.
It uses the c-ares library (when available) to perform asynchronous DNS queries and process the results.

## CREATION

### `static DnsResolving *create()`

Creates a new instance of the **DnsResolving()** class.
Memory is allocated using `new` with the `std::nothrow` modifier to ensure safe handling of failed allocations.

## CONFIGURATION

### `void hostnameSet(const std::string &hostname)`

Sets the hostname to be resolved. This hostname is used to query both IPv4 and IPv6 addresses.

- **hostname**: The domain name to resolve (e.g., "example.com").

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

### `const std::list<std::string> &lstIPv4()`

Returns a list of resolved IPv4 addresses for the set hostname.

### `const std::list<std::string> &lstIPv6()`

Returns a list of resolved IPv6 addresses for the set hostname.

## ERRORS

**Note**: Error codes may not be distinctly defined at this time.

Possible causes and their corresponding error codes identifiers are:

```
    Code                   Cause

    <none>                 Dependency libc-ares is not met
    <none>                 libc-ares encountered an error during
                           DNS resolution
```

## REPEL

### `Processing *repel(Processing *pChild)`

After a process has completed and its results have been consumed, the process must be separated from the parent process using the **repel()** function. This is inherent to the **nature of processes**.

## EXAMPLES

### Example: Simple DNS Resolution

In the header file of **ServerConnecting()**
```cpp
  /* member variables */
  DnsResolving *mpResolv;
```

In the source file of **ServerConnecting()**
```cpp
ServerConnecting::ServerConnecting()
  : Processing("ServerConnecting")
  , mpResolv(NULL) // initialize pointer
{
  mState = StStart;
}

Success ServerConnecting::process()
{
  Success success;

  switch (mState)
  {
  case StStart:

    // create AND CHECK
    mpResolv = DnsResolving::create()
    if (!mpResolv)
      return procErrLog(-1, "could not create process");

    // configure
    mpResolv->hostnameSet("example.com");

    // start
    start(mpResolv);

    mState = StDnsResolvDoneWait;

    break;
  case StDnsResolvDoneWait:

    // wait
    success = mpResolv->success();
    if (success == Pending)
      break;

    // FIRST: check for errors
    if (success != Positive)
      return procErrLog(-1, "could not finish DNS request");

    // consume result
    for (list<string>::iterator iter = mpResolv->lstIPv4().begin();
                              iter != mpResolv->lstIPv4().end(); ++iter)
      procInfLog("IPv4 address: %s", iter->c_str());

    // ALWAYS: repel
    repel(mpResolv);
    mpResolv = NULL;

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
- Windows
- FreeBSD
- MacOSX

## RECURSION

```
Order                 1
Depth                 -
```

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

### libc-ares

This library provides asynchronous DNS request handling in C, enabling programs to perform DNS lookups without blocking. It allows querying for various DNS records like A, AAAA, and CNAME, and is ideal for networking applications by allowing DNS operations to be handled in a non-blocking fashion.

```
License               MIT
Required              Yes
Project Page          https://c-ares.org
Documentation         https://c-ares.org/docs.html
Sources               https://github.com/c-ares/c-ares
```

## SEE ALSO

**Processing()**, **c-ares**, **ares_getaddrinfo()**, **getaddrinfo()**

## COPYRIGHT

Copyright (C) 2024, Johannes Natter

## LICENSE

This program is distributed under the terms of the GNU General Public License v3 or later. See <http://www.gnu.org/licenses/> for more information.

