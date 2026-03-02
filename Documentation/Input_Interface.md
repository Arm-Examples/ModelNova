# Input interface

This section explains how the input interface with signal conditioning is created for the [Alif E8 AppKit](https://www.keil.arm.com/boards/alif-semiconductor-appkit-e8-aiml-a-b437af7/features/).

## Overview

The file `sds_main.c` implements the inference loop. This is the pseudo code of the operation.

```c
  while (1)  {
    GetInputData ();       // Get a camera image as required by AI model
    sdsRecWrite ();        // Record the camera image in a SDS data file
    ExecuteAlgorithm ();   // Execute the AI model inference, outputs RPS classification result
    sdsRecWrite ();        // Record the AI model output in a SDS data file
  }
```

The overall data flow of the complete application is:

Data Flow                    | Where     | Description
:---------------------------:|:---------:|:--------------------------------------------------
Camera Input<br> ▼           |Layer Board| Software component `Device:SOC Peripherals:CPI` implements the camera interface.
vstream_video_in<br> ▼       |Layer Board| Source file `vstream_video_in.c` converts the camera input to a data stream.
GetInputData<br> ▼           |  Project  | Source file `sds_data_in_user.c` implements `GetInputData()` which gets a camera image and converts it into the AI model input.
ExecuteAlgorithm<br> ▼       |  Project  | Source file `sds_algorithm_user.cpp` implements `ExecuteAlgorithm()` which calls `run_inference`.
run_inference<br> ▼          |  Project  | Source file `arm_executor_runner.cc` implements `run_inference ()` that pushes current input tensor bytes into ExecuTorch and runs execute
execute<br> ▼                |Layer AI Model| Executes the AI model (the method execute is part of ExecuTorch).
postprocess                  |  Project  | Source file `sds_algorithm_user.cpp` calls `postprocess()` to print the results.

## GetInputData

The file `sds_data_in_user.c` implements the input interface of the AI model and effectively the 

The function `GetInputData()` is called once per inference cycle to produce one input block for the AI model. It starts a single-shot video capture, blocks using an RTOS thread flag until a frame is available, retrieves the raw camera frame, converts and crops/resizes it to the AI model input dimensions (`ML_IMAGE_WIDTH × ML_IMAGE_HEIGHT × 3` bytes), then releases the frame buffer.

- The camera format is configured in the file `algorithm\config_video.h`. 
- The AI model format is configured in the file `algorithm\config_ml_model.h`

## Capture Input Data

To capture input data, build with the [project `AlgorithmTest` with Build Type: DebugRec](https://arm-software.github.io/SDS-Framework/main/template.html#build-the-template-application).

When running the application on the target, you may capture the input data with the SDS-Framework. Use this steps:

1. Start [SDSIO-Server](https://arm-software.github.io/SDS-Framework/main/utilities.html#sdsio-server) on your host computer with `sdsio-server usb`
2. Connect the host computer to the SOC USB
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

Using the [SDS-Convert](https://arm-software.github.io/SDS-Framework/main/utilities.html#sds-convert) with a configured [image metadata](https://arm-software.github.io/SDS-Framework/main/theory.html#image-metadata-format) file (available in `RockPaperScissors\SDS_Metadata`) allows to convert the data stream into a MP4 video file.

```txt
>sds-convert video -i DataInput.0.sds -o DataOutput.0.mp4 -y RockPaperScissors/SDS_Metadata/RGB888.sds.yml
Video conversion complete: 30 frames written to DataOutput.0.mp4
```

