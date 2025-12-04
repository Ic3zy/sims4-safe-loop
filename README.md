# 🚀 Advanced Main-Thread Execution Loop System

This project provides a highly safe, low-overhead C-extension for executing periodic Python callbacks exclusively on the main thread of the host application.

It is designed to be **100% thread-safe** and **asynchronous (async 0)**, ensuring high performance without blocking the main application thread.

---

### ✨ Key Features & Benefits

* **Main-Thread Safety:** Callback execution is strictly performed on the application's main Python thread using `Py_AddPendingCall()`. This makes it inherently safe for engine-bound systems such as UI, services, and simulation code.
    * *Crucial for embedded runtimes like The Sims 4's `python37_x64.dll`.*
* **Zero CPU Overhead:** The worker thread is non-blocking. It exclusively calls `Sleep()` and never uses busy-waiting, ensuring **zero CPU load** and no performance impact on the main thread.
* **Dynamic Interval Control:** The execution interval can be determined dynamically at runtime via a custom `interval()` callback function.
* **Simple Python Interface:** Easy-to-use API: `Loop(func, interval)`.
* **High Performance:** Implemented as an optimized C-extension (`.pyd`) for maximum speed and efficiency.
* **Clean Management:** Includes robust `start()`, `stop()`, and proper deallocation mechanisms.

---

### 💻 Python Usage Example

The system requires two components: the function to execute (`tick`) and a function to dynamically determine the interval (`interval`).

```python
from loopmod import Loop
import time

# The function to be called periodically
def tick():
    print("Tick at:", time.time())

# The function that returns the desired interval (in seconds)
def interval():
    return 0.2  # 200 ms

# Create and start the loop
loop = Loop(tick, interval)
loop.start()

# To stop the loop later in the runtime:
# loop.stop()
```

---

### 📦 Releases and Targets

This repository supports two separate build targets, catering to different Python environments:

| Target | Description | Required DLL |
| :--- | :--- | :--- |
| **Standard Python 3.7** | Intended for testing and general use within official CPython 3.7 environments. | `python37.dll` |
| **The Sims 4 Safe Release** | Required for integration with **The Sims 4's** embedded Python runtime. Guarantees safety for all game systems. | `python37_x64.dll` |

---

### 🛠️ Build Guide — Standard Python 3.7

This target links against the official CPython `python37.dll`.

**Requirements:**
* Visual Studio Developer Command Prompt
* Official CPython `python37.dll` and `python37.lib`
* Python 3.7 include directory

**Compilation Command:**

```bash
cl /LD loopmod.c python37.lib /I "C:\Python37\include" /link /OUT:loopmod.pyd
```

---

### 🎮 Build Guide — The Sims 4 (`python37_x64.dll`)

Building for The Sims 4's embedded runtime requires generating a custom import library since the official `.lib` file for `python37_x64.dll` is not provided.

#### 1. Locate the DLL

Find the DLL in your game installation directory:

> `F:\The Sims 4\Game\Bin\python37_x64.dll`

#### 2. Create the Definition File (`.def`)

Create a file named **`python37_x64.def`** containing the following required Python symbols:

```def
LIBRARY python37_x64.dll
EXPORTS
    Py_AddPendingCall
    PyErr_Print
    PyFloat_AsDouble
    PyObject_CallObject
    PyType_Ready
    PyModule_Create2
    PyModule_AddObject
    Py_XDECREF
    Py_INCREF
```

#### 3. Generate the Import Library

Use the `lib` utility to create the import library (`.lib`) from the definition file:

```bash
lib /def:python37_x64.def /out:python37_x64.lib /machine:x64
```

#### 4. Compile the Sims 4 Compatible `.pyd`

Compile the C-extension, linking against the custom library and ensuring you skip the default `python37.lib` dependency:

```bash
cl /LD loopmod.c python37_x64.lib /I "C:\Python37\include" /link /OUT:loopmod.pyd /nodefaultlib:python37.lib
```

#### 5. Installation and Import

Place the generated **`loopmod.pyd`** into an appropriate TS4 Mods folder:

> `Documents\Electronic Arts\The Sims 4\Mods\loop\Scripts\loopmod.pyd`

Import it within your Sims 4 Python scripts:

```python
from loopmod import Loop
```
