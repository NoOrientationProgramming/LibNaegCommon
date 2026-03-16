
# MailSending() Manual Page

## ABSTRACT

Class for sending emails using SMTP.

## LIBRARY

LibNaegCommon

## SYNOPSIS

```cpp
#include "MailSending.h"

// creation
static MailSending *create();

// configuration
void serverSet(const std::string &server);
void passwordSet(const std::string &password);
void recipientSet(const std::string &recipient);
void senderSet(const std::string &sender);
void subjectSet(const std::string &subject);
void bodySet(const std::string &body);

// start / cancel
Processing *start(Processing *pChild, DriverMode driver = DrivenByParent);
Processing *cancel(Processing *pChild);

// success
Success success();

// result
// environment change -> email sent

// repel
Processing *repel(Processing *pChild);
Processing *whenFinishedRepel(Processing *pChild);
```

## DESCRIPTION

The **MailSending()** class provides functionality for sending emails via the SMTP protocol. It utilizes the cURL library for handling email transmission over SMTP, offering both synchronous and asynchronous operations.

## CREATION

### `static MailSending *create()`

Creates a new instance of the **MailSending()** class. Memory is allocated using `new` with the `std::nothrow` modifier to ensure safe handling of failed allocations.

## CONFIGURATION

### `void serverSet(const std::string &server)`

Sets the SMTP server to be used for sending the email.

- **server**: The SMTP server address (e.g., "smtp.example.com").

### `void passwordSet(const std::string &password)`

Sets the password for SMTP authentication.

- **password**: The password for the SMTP server.

### `void recipientSet(const std::string &recipient)`

Sets the email recipient address.

- **recipient**: The recipient's email address (e.g., "recipient@example.com").

### `void senderSet(const std::string &sender)`

Sets the email sender address.

- **sender**: The sender's email address (e.g., "sender@example.com").

### `void subjectSet(const std::string &subject)`

Sets the subject of the email.

- **subject**: The email subject.

### `void bodySet(const std::string &body)`

Sets the body content of the email.

- **body**: The email body.

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

Environment change. An email has been sent to the recipient.

## ERRORS

**Note**: Error codes may not be distinctly defined at this time.

Possible causes and their corresponding error codes identifiers are:

```
    Code                   Cause

    <none>                 Bad configuration
    <none>                 Timeout sending mail
    <none>                 Error during transfer
```

## REPEL

### `Processing *repel(Processing *pChild)`

After a process has completed and its results have been consumed, the process must be separated from the parent process using the **repel()** function. This is inherent to the **nature of processes**.

## EXAMPLES

### Example: Simple Email Sending

In the header file of **MsgDispatching()**
```cpp
  /* member variables */
  MailSending *mpMail;
```

In the source file of **MsgDispatching()**
```cpp
MsgDispatching::MsgDispatching()
  : Processing("MsgDispatching")
  , mpMail(NULL) // initialize pointer
{
  mState = StStart;
}

Success MsgDispatching::process()
{
  Success success;

  switch (mState)
  {
  case StStart:

    // create AND CHECK
    mpMail = MailSending::create();
    if (!mpMail)
      return procErrLog(-1, "could not create process");

    // configure (required)
    mpMail->serverSet("smtp.example.com");
    mpMail->recipientSet("recipient@example.com");
    mpMail->senderSet("sender@example.com");
    mpMail->subjectSet("Test Email");
    mpMail->bodySet("This is a test email.");

    // start
    start(mpMail);

    mState = StMailDoneWait;

    break;
  case StMailDoneWait:

    // wait
    success = mpMail->success();
    if (success == Pending)
      break;

    // FIRST: check for errors
    if (success != Positive)
      procWrnLog("could not finish email sending");

    // ALWAYS: repel
    repel(mpMail);
    mpMail = NULL;

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

cURL is a library for transferring data with URLs, supporting SMTP for sending emails.

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

