
# EspLedPulsing() Manual Page

## ABSTRACT

Controlling LED pulsing effects for a specified GPIO pin on ESP32.

## LIBRARY

LibNaegCommon

## SYNOPSIS

```cpp
#include "EspLedPulsing.h"

// creation
static EspLedPulsing *create();

// configuration
void pinSet(uint8_t id);
void paramSet(uint32_t width, uint32_t period, uint8_t count = 1, uint32_t gap = 0);

// start / cancel
Processing *start(Processing *pChild, DriverMode driver = DrivenByParent);
Processing *cancel(Processing *pChild);

// success
Success success();

// repel
Processing *repel(Processing *pChild);
Processing *whenFinishedRepel(Processing *pChild);
```

## DESCRIPTION

The **EspLedPulsing()** class allows for the creation of LED pulsing effects on a specified GPIO pin. It can configure the pulsing parameters, such as width, period, and gap between pulses, making it suitable for creating visual effects.

## CREATION

### `static EspLedPulsing *create()`

Creates a new instance of the **EspLedPulsing()** class. Memory is allocated using `new` with the `std::nothrow` modifier to ensure safe handling of failed allocations.

## CONFIGURATION

### `void pinSet(uint8_t id)`

Sets the GPIO pin to be used for LED pulsing.

- **id**: The GPIO pin number (e.g. `2`).

### `void paramSet(uint32_t width, uint32_t period, uint8_t count = 1, uint32_t gap = 0)`

Configures the pulsing parameters.

- **width**: The duration of each pulse in milliseconds.
- **period**: The total cycle time of the pulse in milliseconds.
- **count**: The number of pulses to produce (default is `1`).
- **gap**: The gap between the pulses in milliseconds.

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

## REPEL

### `Processing *repel(Processing *pChild)`

After a process has completed and its results have been consumed, the process must be separated from the parent process using the **repel()** function. This is inherent to the **nature of processes**.

## EXAMPLES

### Example: Simple LED Pulsing

In the header file of **EspControlling()**
```cpp
  /* member variables */
  EspLedPulsing *mpLedPulsing;
```

In the source file of **EspControlling()**
```cpp
EspControlling::EspControlling()
  : Processing("EspControlling")
  , mpLedPulsing(NULL) // initialize pointer
{
  mState = StStart;
}

Success EspControlling::process()
{
  Success success;

  switch (mState)
  {
  case StStart:

    // create AND CHECK
    mpLedPulsing = EspLedPulsing::create();
    if (!mpLedPulsing)
    {
      // this may not be fatal
      procWrnLog("could not create process");

      mState = StMain;
      break;
    }

    // configure
    mpLedPulsing->pinSet(2);
    mpLedPulsing->paramSet(500, 1000, 3, 200); // width, period, count, gap

    // start
    start(mpLedPulsing);

    mState = StMain;

    break;
  case StMain:

    ...

    break;
  default:
    break;
  }

  return Pending;
}
```

## SCOPE

- ESP32

## RECURSION

```
Order                 1
Depth                 -
```

## NOTES

**EspLedPulsing()** can be used as a service process.
This means that the lifespan of this process can be the same as that of the parent.

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

## SEE ALSO

**Processing()**

## COPYRIGHT

Copyright (C) 2024, Johannes Natter

## LICENSE

This program is distributed under the terms of the GNU General Public License v3 or later. See <http://www.gnu.org/licenses/> for more information.

