# Rock/Paper/Scissors Application

This application demonstrates how to test DSP and ML algorithms using the SDS framework.
It lets you record and play back real-world data streams on physical hardware, feeding
them to your algorithm for testing. The data streams are stored in SDS data files.

## Prerequisites

To run this example:

- Install [Keil Studio for VS Code](https://marketplace.visualstudio.com/items?itemName=Arm.keil-studio-pack) and run a
  Blinky example on the board to verify tool installation.
- Install SDS-Framework pack v3.0.0 or later from:
    - `https://github.com/ARM-software/SDS-Framework/releases`
- Setup the [Python environment](https://arm-software.github.io/SDS-Framework/main/utilities.html#setup) for running
  the SDS Utilities.
- Install Alif Ensemble CMSIS DFP pack v2.2.0 or later with:
    - `cpackget add AlifSemiconductor::Ensemble@2.2.0`

## Alif AppKit-E8-AIML

The [Alif **AppKit-E8-AIML**](https://www.keil.arm.com/boards/alif-semiconductor-appkit-e8-aiml-a-b437af7/features/) features a dual-core Cortex-M55 each paired with an Ethos-U55 NPU. An Ethos-U85 NPU is also available on the device.

Before using this SDS example on the board, it is required to program the ATOC of the device using the Alif SETOOLS.
Refer to the section [Usage](https://www.keil.arm.com/packs/ensemble-alifsemiconductor/overview/) in the overview page
of the Alif Semiconductor Ensemble DFP/BSP for information on how to setup these tools.

In VS Code, use the menu command **Terminal - Run Tasks** and execute:

- `"Alif: Install M55_HE or M55_HP debug stubs (single core configuration)"`

> Note:
>
> - Configure SW4 to position SE (Secure UART) to enable SETOOLS communication with the device.
> - Configure SW4 to position U4 (UART4) to see STDIO messages from the application.

## Project

The `SDS.csolution.yml` application is pre-configured for **AppKit-E8-AIML**. The **`AlgorithmTest.cproject.yml`**
verifies a user algorithm with recording and playback of SDS data files.

## Layer Type: Board and Layer Type: SDSIO

The board layer implements the Hardware Abstraction Layer (HAL).
The SDSIO layer implements the communication layer that communicates with SDSIO Server.

- `Board/AppKit-E8_M55_HP/Board_HP.clayer.yml` provides board/device drivers
- `sdsio/usb/sdsio_usb.clayer.yml` provides the **USB Interface** for SDS I/O communication interface

## Build Types

- **`Debug`**: Debug version of application used for recording of ML algorithm input data (image 224 x 224) and results of ML algorithm,
  or playback of previously recorder ML algorithm input data and recording of results of ML algorithm.
- **`Release`**: Release version of application used for recording of ML algorithm input data (image 224 x 224) and results of ML algorithm,
  or playback of previously recorder ML algorithm input data and recording of results of ML algorithm.

> Note:
>
> The only difference between `Debug` and `Release` targets is compiler optimization level and debug information.

## AlgorithmTest Project

The AlgorithmTest project demonstrates real-world usage of the SDS Framework on an object detection ML model.

This project supports both recording and playback functionality. By default, the application starts in recording mode.
You can switch modes via the SDSIO Server: use `R` key to start recording, `S` key to stop recording, and `P` key to begin playback.

When used in **recording** mode:

- **Captures on-board camera stream** via SDS recording stream (ML_In.<n>.sds file)
- **Executes ML inference** using an object detection ML model
- **Captures algorithm output** via SDS recording stream (ML_Out.<n>.sds file)

When used in **playback** mode:

- **Replays pre-recorded video stream** via SDS playback stream (ML_In.<n>.sds file)
- **Executes ML inference** using an object detection ML model
- **Captures algorithm output** via SDS recording stream (ML_Out.<m>.sds file)

### Setup

Begin by starting the [SDSIO Server](https://arm-software.github.io/SDS-Framework/main/utilities.html#sdsio-server):

- Open Terminal and type `sdsio-server.py usb`.
- Check [SDS Utilities](https://arm-software.github.io/SDS-Framework/main/utilities.html) configuration if SDSIO Server
  is not found.

**SDSIO Server Output:**

```bash
>sdsio-server.py usb
Press Ctrl+C to exit.
Starting SDS Control Flags thread. R=record, P=playback, S/s=stop, X/x=terminate, A-H=set flags 0-7, a-h=clear flags 0-7.
Starting USB Server...
Waiting for SDSIO Client USB device...
```

In VS Code, open the **CMSIS** view to build and run the project using the following steps:

1. **Build solution** to generate the executable image.
2. Connect the **PRG USB (J3)** of the **AppKit-E8-AIML** and configure **SW4** switch for **SETOOLS** (SE position).
3. If not already done, download debug stubs using the task:
   `"Alif: Install M55_HE or M55_HP debug stubs (single core configuration)"`.
4. **Load application to target** to download the application to the board.
5. Configure **SW4** for **UART4** (U4 position) and use the VS Code **Serial Monitor** to view the application output (STDIO).
6. Connect the **MCU USB (J2)** of the **AppKit-E8-AIML** to the **PC** running the **SDSIO Server**.
7. Reset the board with **RESET (SW1)** button and observe the application output (STDIO) like below

```txt
SDS I/O USB interface initialized successfully
Model buffer loaded, has 1 methods
Running method forward
Setup Method allocator pool. Size: 1048576 bytes.
Setting up planned buffer 0, size 752640
Method 'forward' loaded.
Model initialized. Ready for inference.
35% idle
40% idle
 :
```

### Recording Test

To execute the **recording** test, just:

1. Press the **joystick (SW2)** on the board or press `R` key in the SDSIO Server window to start the recording.
2. Press the **joystick (SW2)** again or press `S` key in the SDSIO Server window to stop the recording.

**SDSIO Server Output:**

```bash
>sdsio-server.py usb
Press Ctrl+C to exit.
Starting SDS Control Flags thread. R=record, P=playback, S/s=stop, X/x=terminate, A-H=set flags 0-7, a-h=clear flags 0-7.
Starting USB Server...
SDSIO Client USB device connected.
sdsFlags = 0x10000000
40% idle
 :
Record:   ML_In (c:\SDS\ML_In.0.sds)
Record:   ML_Out (c:\SDS\ML_Out.0.sds)
..............
sdsFlags = 0x10000000
.
Closed:   ML_In (c:\SDS\ML_In.0.sds)
Closed:   ML_Out (c:\SDS\ML_Out.0.sds)
```

**Serial Monitor Output:**

```txt
==== SDS recording started
Post-processed output:
Predicted class : UNKNOWN
Confidence      : 99.51 %
40% idle
...
==== SDS recording stopped
```
Each run records two files: `ML_In.<n>.sds` and `ML_Out.<n>.sds` in the directory where SDSIO Server was started. `<n>` is a sequential number.

#### Check SDS Files

The [SDS Check](https://arm-software.github.io/SDS-Framework/main/utilities.html#sds-check)
utility verifies SDS files for consistency. For example:

```bash
>sds-check.py -s ML_In.0.sds
File Name         : ML_In.0.sds
File Size         : 8.580.552 bytes
Number of Records : 57
Recording Time    : 7 s
Recording Interval: 120 ms
Data Size         : 8.580.096 bytes
Data Block        : 150.528 bytes
Data Rate         : 1.254.400 byte/s
Jitter            : Not detected
Validation passed
```

### Playback Test

To execute the **playback** test, just:

1. Press the `P` key in the SDSIO Server window to start the playback.
2. Press the `S` key in the SDSIO Server window to stop the playback.

The stream `ML_In.<n>.sds` is read back and the algorithm processes this data. The stream `ML_Out.<n>.p.sds` is written.

> Note:
>
> The playback implementation replays recordings as quickly as possible and does not
> account for time slot information. During playback, the ML system receives the same recorded
> input data, so timing information is not relevant in this context.

**SDSIO-Server Output:**

```bash
>sdsio-server.py usb
sdsControl: start playback ('P')
sdsFlags = 0xB0000000
Playback: ML_In (c:\SDS\ML_In.0.sds)
Record:   ML_Out (c:\SDS\ML_Out.0.p.sds)
Closed:   ML_In (c:\SDS\ML_In.0.sds)
.
55% idle
...
13% idle
....
11% idle
.....
Closed:   ML_Out (c:\SDS\ML_Out.0.p.sds)
sdsFlags = 0x30000000
57% idle
```

> Note:
>
> ML_Out file recorded during playback should be identical to the one recorded earlier.

### Key Components

**Video Frame Capture** (`data_in_user.c`):

- Initializes on-board camera input stream using CMSIS vStream driver
- Captures video frames and pre-processes them for use as input to the ML model
- Provides frame data for SDS recording

**Algorithm Processing** (`algorithm_user.cpp`):

- Initializes ML model and LCD display stream using CMSIS vStream driver
- Executes ML inference (pre-processing, inference, post-processing)
- Copies detection results to output buffer for SDS recording
- Displays frames on LCD with object type with confidence percentage
