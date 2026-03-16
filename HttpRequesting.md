
# HttpRequesting() Manual Page

## ABSTRACT

Class for making HTTP requests.

## LIBRARY

LibNaegCommon

## SYNOPSIS

```cpp
#include "HttpRequesting.h"

// creation
static HttpRequesting *create();
static HttpRequesting *create(const std::string &url);

// configuration
void urlSet(const std::string &url);
void typeSet(const std::string &type);
void userPwSet(const std::string &userPw);
void hdrAdd(const std::string &hdr);
void dataSet(const std::string &data);
void authMethodSet(const std::string &authMethod);
void versionTlsSet(const std::string &versionTls);
void versionHttpSet(const std::string &versionHttp);
void modeDebugSet(bool en);

CURL *easyHandleCurl();

// start / cancel
Processing *start(Processing *pChild, DriverMode driver = DrivenByParent);
Processing *cancel(Processing *pChild);

// success
Success success();

// result
uint16_t respCode() const;
std::string &respHdr();
std::string &respData();

// repel
Processing *repel(Processing *pChild);
Processing *whenFinishedRepel(Processing *pChild);
```

## DESCRIPTION

The **HttpRequesting()** class provides functionality for sending HTTP requests and processing their responses. It utilizes the cURL library for handling HTTP operations, allowing for synchronous and asynchronous request handling.

## CREATION

### `static HttpRequesting *create()`

Creates a new instance of the **HttpRequesting()** class. Memory is allocated using `new` with the `std::nothrow` modifier to ensure safe handling of failed allocations.

### `static HttpRequesting *create(const std::string &url)`

Creates a new instance of the **HttpRequesting()** class with a specified URL. Memory is allocated using `new` with the `std::nothrow` modifier to ensure safe handling of failed allocations.

## CONFIGURATION

### `void urlSet(const std::string &url)`

Sets the URL for the HTTP request.

- **url**: The URL to which the request will be sent (e.g., "http://example.com").

### `void typeSet(const std::string &type)`

Sets the type of the HTTP request (e.g., GET, POST).

### `void userPwSet(const std::string &userPw)`

Sets the username and password for authentication.

### `void hdrAdd(const std::string &hdr)`

Adds a custom header to the HTTP request.

- **hdr**: The header string to be added (e.g., "Authorization: Bearer token").

### `void dataSet(const std::string &data)`

Sets the data to be sent with the HTTP request (for methods like POST).

### `void authMethodSet(const std::string &authMethod)`

Sets the authentication method for the request (e.g., Basic, Bearer).

### `void versionTlsSet(const std::string &versionTls)`

Sets the desired TLS version for secure requests.

### `void versionHttpSet(const std::string &versionHttp)`

Sets the HTTP version to be used for the request (e.g., HTTP/1.1).

### `void modeDebugSet(bool en)`

Enables or disables debugging mode for detailed output during the request process.

### `CURL *easyHandleCurl()`

Returns the handle to a transfer in libcurl called _easy handle_.
With this pointer you can perform multiple calls to `curl_easy_setopt()` between
the creation of **HttpRequesting()** (function `create()`) and start of the
process (function `start()`).

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

### `uint16_t respCode() const`

Returns the HTTP response code from the last request.

### `std::string &respHdr()`

Returns the response headers from the last request.

### `std::string &respData()`

Returns the response data from the last request.

## ERRORS

**Note**: Error codes may not be distinctly defined at this time.

Possible causes and their corresponding error codes identifiers are:

```
    Code                   Cause

    <none>                 Dependency cURL is not met
    <none>                 cURL encountered an error during
                           HTTP request
```

## REPEL

### `Processing *repel(Processing *pChild)`

After a process has completed and its results have been consumed, the process must be separated from the parent process using the **repel()** function. This is inherent to the **nature of processes**.

## EXAMPLES

### Example: Simple HTTP Request

In the header file of **ServerConnecting()**
```cpp
  /* member variables */
  HttpRequesting *mpRequest;
```

In the source file of **ServerConnecting()**
```cpp
ServerConnecting::ServerConnecting()
  : Processing("ServerConnecting")
  , mpRequest(NULL) // initialize pointer
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
    mpRequest = HttpRequesting::create("http://example.com");
    if (!mpRequest)
      return procErrLog(-1, "could not create process");

    // configure (optional)
    mpRequest->hdrAdd("User-Agent: my-cool-app");
    mpRequest->modeDebugSet(true);

    {
      CURL *pCurl = mpRequest->easyHandleCurl();
      curl_easy_setopt(pCurl, CURLOPT_CONNECTTIMEOUT, 5L);
    }

    // start
    start(mpRequest);

    mState = StHttpRequestDoneWait;

    break;
  case StHttpRequestDoneWait:

    // wait
    success = mpRequest->success();
    if (success == Pending)
      break;

    // FIRST: check for errors
    if (success != Positive)
      return procErrLog(-1, "could not finish HTTP request");

    // consume result
    procInfLog("Response Code: %d", mpRequest->respCode());
    procInfLog("Response Data: %s", mpRequest->respData().c_str());

    // ALWAYS: repel
    repel(mpRequest);
    mpRequest = NULL;

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

### DnsResolving()

Process for DNS resolution.

```
License               GPLv3
Required              If c-ares is used
Project Page          https://github.com/fractal-programming
Documentation         https://github.com/fractal-programming/LibNaegCommon
Sources               https://github.com/fractal-programming/LibNaegCommon
```

### LibDspc

....

```
License               GPLv3
Required              Yes
Project Page          https://github.com/fractal-programming
Documentation         https://github.com/fractal-programming/LibNaegCommon
Sources               https://github.com/fractal-programming/LibNaegCommon
```

### cURL

cURL is a library for transferring data with URLs. It supports various protocols and provides easy-to-use interfaces for making HTTP requests.

```
License               MIT-like
                      https://curl.se/docs/copyright.html
Required              Yes
Project Page          https://curl.se
Documentation         https://curl.se/libcurl/c/libcurl.html
Sources               https://github.com/curl/curl
```

## SEE ALSO

**Processing()**, **cURL**, **curl_easy_perform()**, **curl_multi_perform()**

## COPYRIGHT

Copyright (C) 2024, Johannes Natter

## LICENSE

This program is distributed under the terms of the GNU General Public License v3 or later. See <http://www.gnu.org/licenses/> for more information.

