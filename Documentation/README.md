# Edge AI Project

The RockPaperScissors project implements an ML model that detects [three hand gestures](https://en.wikipedia.org/wiki/Rock_paper_scissors). It runs on an [Alif AppKit-E8-AIML](https://www.keil.arm.com/boards/alif-semiconductor-appkit-e8-aiml-a-b437af7/features/) that offers camera input and an LCD display. The SoC USB interface connects the board to a host computer for data capture and playback using the SDS Framework.

## Overview

The project uses software layers to decouple functionality as shown in the diagram below. The target type `AppKit-E8-U85` or `SSE-320-U85` selects the board layer for hardware or the simulation model. Using a different AI/ExecuTorch layer would change the ML model behavior.

![Project Structure](./image/Project-Structure.png "Project Structure")

The application uses standardized interfaces that are provided by the board layer `Board/AppKit-E8_M55_HP/Board_HP-U85.clayer.yml`.

- [CMSIS-Driver vStream](https://arm-software.github.io/CMSIS_6/latest/Driver/group__vstream__interface__gr.html) is an interface for streaming data with fixed-size data blocks. It is used for camera input (source file `vstream_video_in.c`) and optionally for LCD output (source file `vstream_video_out.c`).
- [CMSIS-Driver USB Device](https://arm-software.github.io/CMSIS_6/latest/Driver/group__usbd__interface__gr.html) implements the USB communication interface to the SDSIO-Server. It is provided by the component `CMSIS Driver:USB Device`.

The file `sds_main.c` implements the inference loop. This is the pseudocode for the operation.

```c
  while (1)  {
    GetInputData ();       // Get a camera image as required by the ML model
    sdsRecWrite ();        // Record the camera image in an SDS data file
    ExecuteAlgorithm ();   // Execute ML model inference and output RPS classification result
    sdsRecWrite ();        // Record the ML model output in an SDS data file
  }
```

![Application Structure](./image/Application-Structure.png "Application Structure")

The overall data flow of the application is:

Data Flow                     | Where          | Description
:----------------------------:|:--------------:|:--------------------------------------------------
Camera input<br/>▼            | Layer Board    | Software component `Device:SOC Peripherals:CPI` implements the camera interface.
vstream_video_in<br/>▼        | Layer Board    | Source file `vstream_video_in.c` converts the camera input to a data stream.
GetInputData<br/>▼            |  Project       | Source file `sds_data_in_user.c` implements `GetInputData()`, which gets a camera image and converts it into the ML model input.
ExecuteAlgorithm<br/>▼        |  Project       | Source file `sds_algorithm_user.cpp` implements `ExecuteAlgorithm()`, which calls `run_inference()`.
run_inference<br/>▼           |  Project       | Source file `arm_executor_runner.cc` implements `run_inference()`, which pushes the current input tensor bytes into ExecuTorch and runs `execute()`.
execute<br/>▼                 | Layer AI / Executorch | Executes the ML model (the `execute` method is part of ExecuTorch).
postprocess                   |  Project       | Source file `sds_algorithm_user.cpp` calls `postprocess()` to print the results.

The project runs on the [Alif AppKit-E8-AIML](https://www.keil.arm.com/boards/alif-semiconductor-appkit-e8-aiml-a-b437af7/features/).

- The USB SoC interface connects to the [SDSIO-Server](https://arm-software.github.io/SDS-Framework/main/utilities.html#sdsio-server) and is used to record and play back SDS files.
- The USB Debug/UART interface connects via J-Link to the [Keil Studio Debugger](https://marketplace.visualstudio.com/items?itemName=Arm.keil-studio-pack) and via [Serial Monitor](https://marketplace.visualstudio.com/items?itemName=ms-vscode.vscode-serial-monitor) to the application's UART output.
- Optionally, a CMSIS-DAP adapter (for example, ULINKplus) can be used with pyOCD for debugging and testing. This setup is used to [measure timing](#measure-timing) and run [regression testing](#regression-test) in a hardware-in-the-loop (HIL) system.

![Alif AppKit-E8-AIML Connectors](./image/AppKit.png "Alif AppKit-E8-AIML Connectors")

## Input Interface and Signal Conditioning

This section explains how the camera input interface with signal conditioning is implemented for the [Alif E8 AppKit](https://www.keil.arm.com/boards/alif-semiconductor-appkit-e8-aiml-a-b437af7/features/).

### GetInputData

The file `sds_data_in_user.c` implements the ML model input interface and signal conditioning. In this project, it crops and resizes the camera input.

The function `GetInputData()` is called once per inference cycle to produce one input block for the ML model. It starts a single-shot video capture, waits on an RTOS thread flag until a frame is available, retrieves the raw camera frame, converts it, crops/resizes it to the ML model input dimensions (`ML_IMAGE_WIDTH × ML_IMAGE_HEIGHT × 3` bytes), then releases the frame buffer.

- The camera format is configured in the file `algorithm/config_video.h`.
- The ML model format is configured in the file `algorithm/config_ml_model.h`.

### Capture ML model Input Data

To capture ML model input data, build the project `AlgorithmTest` with [Build Type: DebugRec](https://arm-software.github.io/SDS-Framework/main/template.html#build-the-template-application).

When running the application on the target, you may capture the input data with the SDS Framework. Use these steps:

1. Start [SDSIO-Server](https://arm-software.github.io/SDS-Framework/main/utilities.html#sdsio-server) on your host computer with `sdsio-server usb`.
2. Connect the host computer to the SoC USB port.
3. Start and stop recording using the joystick push-button, or by sending the `s` character to the board via the STDIO (UART4) interface.

**Example console output:**

```txt
>sdsio-server usb
Press Ctrl+C to exit.
Starting USB Server...
Waiting for SDSIO Client USB device...
SDSIO Client USB device connected.
Ping received.
Record:   ML_In (.\ML_In.0.sds).
............
Closed:   ML_In (.\ML_In.0.sds).
```

Using [SDS-Convert](https://arm-software.github.io/SDS-Framework/main/utilities.html#sds-convert) with a configured [image metadata](https://arm-software.github.io/SDS-Framework/main/theory.html#image-metadata-format) file (available in `RockPaperScissors/SDS_Metadata`) allows you to convert the data stream into an MP4 video file.

```txt
>sds-convert video -i ML_In.0.sds -o ML_In.0.mp4 -y RockPaperScissors/SDS_Metadata/RGB888.sds.yml
Video conversion complete: 30 frames written to ML_In.0.mp4
```

> [!TIP]
> Open the generated MP4 file with a video player to review the captured camera images.

## Create ML model

This section explains how the ML model is created with the ModelNova Fusion Studio.

ToDo (i.e.)

- select model
- use training data
- create an optimized model for deployment to Cortex-M/Ethos-U microcontrollers.
- view report
- obtain the ML model

## Integrate ML model

The folder `ai_layer` contains the ML model that is created as described above. The integration is based on the PyTorch [Arm Ethos-U NPU backend](https://docs.pytorch.org/executorch/1.0/backends-arm-ethos-u.html). The file `sds_algorithm_user.cpp` implements `ExecuteAlgorithm()`, which calls `run_inference()` in [`arm_executor_runner.cc`](https://github.com/pytorch/executorch/tree/main/examples/arm/executor_runner), which is derived from PyTorch. This file then calls the `execute` method in `ai_layer`.

### Check ML model Performance

The ML model input data and output data can now be verified. Run the project `AlgorithmTest` with [Build Type: DebugRec](https://arm-software.github.io/SDS-Framework/main/template.html#build-the-template-application) on the target hardware and use `sdsio-server usb` to capture both the `ML_In` and `ML_Out` SDS files.

**Example console output:**

```txt
>sdsio-server usb
  :
Record:   ML_In (.\ML_In.1.sds).
Record:   ML_Out (.\ML_Out.1.sds).
................
Closed:   ML_In (.\ML_In.1.sds).
Closed:   ML_Out (.\ML_Out.1.sds).
```

The ML model delivers four float values for each category `PAPER`, `ROCK`, `SCISSORS`, and `UNKNOWN`. The inference interval is about 8 Hz. This information is recorded in the `ML_Out.<n>.sds` files in binary format. The file `RockPaperScissors/SDS_Metadata/ML_Out.sds.yml` describes the content and using this file with SDS utilities allows you to view the content.

**Example output:**

```txt
>sds-view -s ml_out.1.sds -y RockPaperScissors/SDS_Metadata/ML_Out.sds.yml
```

![SDS-View Output](./image/SDS-View-Output.png "SDS-View Output")

### Measure Timing

Timing can be measured with the SEGGER [SystemView](https://www.segger.com/products/development-tools/systemview/) tool.

The application is instrumented with SystemView markers used to measure timing and log messages for additional information. The instrumented application is available in the [systemview](https://github.com/Arm-Examples/ModelNova/tree/sysview) branch of the project.

Following operations are measured:

- Camera Input
- Algorithm execution: Pre-processing, Inference, Post-processing
- Display Output

Ethos-U PMU (Performance Monitoring Unit) is also used in `inference` to measure time used by the CPU and the NPU. This information is provided as additional log messages.

SystemView data can be captured with a CMSIS-DAP adapter and pyOCD. Use these steps:

- Build the application.
- Program the application to the target.
- Run the task `pyOCD Run` which starts the application and captures SystemView data.
- Stop the task by pressing Ctrl+C. SystemView data is then saved into a *.SVDat file in the output folder.
- Open the *.SVDat file with the SystemView tool to review the timing information.

![System View](./image/SystemView.png "System View")

## Capture New Data

This section explains how additional training data and ML model output data are captured with the SDS framework.

## Retrain ML model

This section explains how the ML model is retrained with additional training data.

## Regression Test

Regression testing verifies that an updated ML model still matches a fixed reference dataset before deployment. In this setup, recorded inputs (`ML_In.*.sds`) are replayed through SDSIO and the results are compared against the reference outputs (`ML_Out.*.sds`) within a defined tolerance. Run this after every retrain to catch accuracy regressions from architecture, training data, or quantization changes.

The build type `DebugPlay` configures the application for SDS data playback and includes only a subset of the application as outlined in the picture below.

![Regression Test - Data Playback](./image/Regression_Test_Playback.png "Regression Test - Data Playback")

There are two possible target types for SDS data playback:

- Target type `SSE-320-U85` runs on the Corstone-320 FVP simulation model and verifies the correctness of the operation. Simulation models are easy to deploy and do not require any hardware.

- Target type `AppKit-E8-U85` runs on the Alif AppKit-E8 target hardware and uses pyOCD with a CMSIS-DAP unit for hardware-in-the-loop (HIL) testing. Besides correctness, timing can also be verified by capturing an RTT file for the SEGGER SystemView tool.
