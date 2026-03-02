# Edge AI Project

The RockPaperScissors project implements an AI model that detects [three hand gestures](https://en.wikipedia.org/wiki/Rock_paper_scissors). It runs on an [Alif AppKit-E8-AIML](https://www.keil.arm.com/boards/alif-semiconductor-appkit-e8-aiml-a-b437af7/features/) that offers camera input and an LCD display. The SoC USB interface connects the board to a host computer for data capture and playback using the SDS Framework.

## Overview

The project uses software layers to decouple functionality as shown in the diagram below. The target type `AppKit-E8-U85` or `SSE-320-U85` changes the board layer to hardware or simulation model. Using a different AI / ExecuTorch layer would change the AI model behavior.

![Project Structure](Project-Structure.png "Project Structure")

The application uses standardized interfaces that are provided by the board layer `Board/AppKit-E8_M55_HP/Board_HP-U85.clayer.yml`.

- [CMSIS-Driver vStream](https://arm-software.github.io/CMSIS_6/latest/Driver/group__vstream__interface__gr.html) is an interface for streaming data with fixed-size data blocks. It is used for camera input (source file `vstream_video_in.c`) and optionally for LCD output (source file `vstream_video_out.c`).
- [CMSIS-Driver USB Device](https://arm-software.github.io/CMSIS_6/latest/Driver/group__usbd__interface__gr.html) implements the USB communication interface to the SDSIO-Server. It is provided by the component `CMSIS Driver:USB Device`.

The file `sds_main.c` implements the inference loop. This is the pseudocode of the operation.

```c
  while (1)  {
    GetInputData ();       // Get a camera image as required by the AI model
    sdsRecWrite ();        // Record the camera image in an SDS data file
    ExecuteAlgorithm ();   // Execute AI model inference and output RPS classification result
    sdsRecWrite ();        // Record the AI model output in an SDS data file
  }
```

![Application Structure](Application-Structure.png "Application Structure")

The overall data flow of the application is:

Data Flow                     | Where     | Description
:----------------------------:|:---------:|:--------------------------------------------------
Camera input<br/>▼            | Layer Board | Software component `Device:SOC Peripherals:CPI` implements the camera interface.
vstream_video_in<br/>▼        | Layer Board | Source file `vstream_video_in.c` converts the camera input to a data stream.
GetInputData<br/>▼            |  Project   | Source file `sds_data_in_user.c` implements `GetInputData()`, which gets a camera image and converts it into the AI model input.
ExecuteAlgorithm<br/>▼        |  Project   | Source file `sds_algorithm_user.cpp` implements `ExecuteAlgorithm()`, which calls `run_inference()`.
run_inference<br/>▼           |  Project   | Source file `arm_executor_runner.cc` implements `run_inference()`, which pushes the current input tensor bytes into ExecuTorch and runs `execute()`.
execute<br/>▼                 | Layer AI Model | Executes the AI model (the `execute` method is part of ExecuTorch).
postprocess                   |  Project   | Source file `sds_algorithm_user.cpp` calls `postprocess()` to print the results.

## Input interface and Signal Conditioning

This section explains how the input interface with signal conditioning is implemented for the [Alif E8 AppKit](https://www.keil.arm.com/boards/alif-semiconductor-appkit-e8-aiml-a-b437af7/features/).

### GetInputData

The file `sds_data_in_user.c` implements the AI model input interface and the signal conditioning; in this case, it crops and resizes the camera input.

The function `GetInputData()` is called once per inference cycle to produce one input block for the AI model. It starts a single-shot video capture, blocks using an RTOS thread flag until a frame is available, retrieves the raw camera frame, converts and crops/resizes it to the AI model input dimensions (`ML_IMAGE_WIDTH × ML_IMAGE_HEIGHT × 3` bytes), then releases the frame buffer.

- The camera format is configured in the file `algorithm/config_video.h`.
- The AI model format is configured in the file `algorithm/config_ml_model.h`.

### Capture AI Model Input Data

To capture AI model input data, build the project `AlgorithmTest` with [Build Type: DebugRec](https://arm-software.github.io/SDS-Framework/main/template.html#build-the-template-application).

When running the application on the target, you may capture the input data with the SDS Framework. Use these steps:

1. Start [SDSIO-Server](https://arm-software.github.io/SDS-Framework/main/utilities.html#sdsio-server) on your host computer with `sdsio-server usb`.
2. Connect the host computer to the SoC USB.
3. Start/stop recording using the Joystick push button, or by sending the `s` character to the board via the STDIO (UART4) interface.

**Example console output:**

```txt
>sdsio-server usb
Press Ctrl+C to exit.
Starting USB Server...
Waiting for SDSIO Client USB device...
SDSIO Client USB device connected.
Ping received.
Record:   DataInput (.\DataInput.0.sds).
Record:   DataOutput (.\DataOutput.0.sds).
............
Closed:   DataInput (.\DataInput.0.sds).
Closed:   DataOutput (.\DataOutput.0.sds).
```

Using [SDS-Convert](https://arm-software.github.io/SDS-Framework/main/utilities.html#sds-convert) with a configured [image metadata](https://arm-software.github.io/SDS-Framework/main/theory.html#image-metadata-format) file (available in `RockPaperScissors/SDS_Metadata`) allows you to convert the data stream into an MP4 video file.

```txt
>sds-convert video -i DataInput.0.sds -o DataOutput.0.mp4 -y RockPaperScissors/SDS_Metadata/RGB888.sds.yml
Video conversion complete: 30 frames written to DataOutput.0.mp4
```

> [!TIP]
> Open the generated MP4 file with a video player to review the captured camera images.

## Create AI Model

This section explains how the AI model is created with the ModelNova Fusion Studio.

ToDo (i.e.)
- select model
- use training data
- create an optimized model for deployment to Cortex-M/Ethos-U microcontrollers.
- view report
- obtain the AI model

## Integrate AI Model

The folder `ai_layer` contains the AI model that is created as described above.  The integration is based on the PyTorch [Arm Ethos-U NPU backend](https://docs.pytorch.org/executorch/1.0/backends-arm-ethos-u.html). The `sds_algorithm_user.cpp` implements `ExecuteAlgorithm()`, which calls `run_inference()` in the file [`arm_executor_runner.cc`](https://github.com/pytorch/executorch/tree/main/examples/arm/executor_runner) that is derived from PyTorch. This file calls then the method `execute` in the `ai_layer`.

### Check AI Model Performance

The AI model input data and output data can now be verified. Run the project `AlgorithmTest` with [Build Type: DebugRec](https://arm-software.github.io/SDS-Framework/main/template.html#build-the-template-application) on target hardware and use the `sdsio-server usb` to capture the both the `ML_In` and `ML_Out` SDS files.

**Example console output:**

ToDo

### Measure Timing

ToDo

![System View](SystemView.png "System View")

## Capture New Data

This section explains how additional training data and AI model output data are captured with the SDS framework.

## Retrain AI Model

This section explains how the AI model is retrained with additional training data.

## Regression Test

Regression testing verifies that a new or updated AI model still produces correct results on a known reference dataset before it is deployed to the target hardware. In this project, the previously captured SDS recordings (camera images in `ML_In.*.sds` files) are replayed through the SDSIO interface. If the classification results match the reference outputs (in `ML_Out.*.sds` files) within acceptable tolerances, the model is considered regression-free and safe to deploy to the Alif AppKit-E8 board. This step is critical after each retrain cycle as it guards against inadvertent accuracy loss caused by changes to the model architecture, training data, or quantization settings.

ToDo