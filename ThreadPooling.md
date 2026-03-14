
# THREAD_POOLING() Manual Page

## NAME
**ThreadPooling** – Manages a pool of threads to handle concurrent processing tasks in C++.

## SYNOPSIS
```cpp
#include "ThreadPooling.h"

ThreadPooling *create();

void cntWorkerSet(uint16_t cnt);
void driverCreateSet(FuncDriverPoolCreate pFctDriverCreate);
static void procAdd(Processing *pProc, int32_t idDriver = -1);
```

## DESCRIPTION
**ThreadPooling()** is a C++ class designed to manage a pool of threads efficiently, allowing for the parallel execution of processing tasks. The class provides mechanisms to create, manage, and coordinate worker threads that interact with processing objects and handle tasks.

### Features:
- **Thread Management**: Configure the number of active worker threads using `cntWorkerSet()`.
- **Dynamic Task Processing**: Add processing objects to the pool for execution with `procAdd()`.
- **Extensibility**: Allows customization of the driver creation process through the `driverCreateSet()` function to meet specific requirements.
- **Safe Interaction**: Utilizes mutex protection mechanisms to synchronize access to shared resources.

### Structs:
- **PoolRequest**: A structure that manages processing requests along with the associated processing objects and desired driver IDs.

## METHODS

### Pool Management
- **create()**  
  Allocates a new instance of the **ThreadPooling()** class.

- **cntWorkerSet(uint16_t cnt)**  
  Sets the number of worker threads in the pool.

- **driverCreateSet(FuncDriverPoolCreate pFctDriverCreate)**  
  Sets the function used for creating drivers.

- **procAdd(Processing *pProc, int32_t idDriver = -1)**  
  Adds a processing object to the queue to be handled by the pool.

### Process Management
- **process()**  
  Executes the logic for handling pool requests and managing worker threads.

- **shutdown()**  
  Shuts down all active threads and releases resources.

### Internal Management
- **poolRequestsProcess()**  
  Processes incoming requests in the pool.

- **procsDrive()**  
  Manages the ongoing processing objects in the pool.

- **idDriverNextGet()**  
  Returns the ID of the next driver to be used in the pool.

- **numProcessingGet()**  
  Returns the number of currently processed objects in the pool.

- **procInternalAdd(Processing *pProc)**  
  Adds an internal processing object to the internal list.

## RETURN VALUES
Methods that modify the status or configuration typically return `Success` to indicate the successful completion of the operation. Functions that return information provide specific values, such as the number of currently processed tasks.

## NOTES
- This class uses a broker mechanism to manage communication between threads and efficiently manage resources.
- The class is not copyable or assignable to prevent unintended sharing of resources or duplication.

## SEE ALSO
- `Processing()`, `mutex`, `thread`

## AUTHORS
Written by Johannes Natter.

