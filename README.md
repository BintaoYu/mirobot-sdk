# Steel

## Build Instructions
To build the project for a specific platform, use the following commands:

```bash
make PLATFORM=linux
make PLATFORM=windows
make PLATFORM=arm
make PLATFORM=raspberrypi
```

To clean the build:
```bash
make clean
```

## Communication Methods
This project supports multiple communication methods through a unified interface:
- **Serial Communication**: Traditional serial port communication.
- **TCP Communication**: Communication over TCP/IP.
- **USB Communication**: Communication over USB.

### Configuration
The communication method can be selected dynamically in the code using the `createComm` factory function. Example:

```cpp
#include "comm_interface.h"

std::unique_ptr<CommInterface> comm = createComm("serial");
comm->open();
comm->write("Hello", 5);
comm->close();
```

### Directory Structure
- `src/`: Contains source code for communication methods and platform-specific implementations.
- `include/`: Contains header files for the communication interface and implementations.
- `platform/`: Contains platform-specific code (e.g., Linux, Windows).

### Adding New Communication Methods
To add a new communication method:
1. Implement the `CommInterface` in a new class (e.g., `NewComm`).
2. Add the implementation to the `createComm` factory function in `comm_interface.cpp`.
3. Update the `Makefile` to include the new source files.