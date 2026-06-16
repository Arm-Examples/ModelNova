# Edge AI Project

The RockPaperScissors project implements an ML model that detects [three hand gestures](https://en.wikipedia.org/wiki/Rock_paper_scissors). It runs on an [Alif AppKit-E8-AIML](https://www.keil.arm.com/boards/alif-semiconductor-appkit-e8-aiml-a-b437af7/features/) that offers camera input and an LCD display. The SoC USB interface connects the board to a host computer for data capture and playback using the SDS Framework.

## Overview

The project uses software layers to decouple functionality as shown in the diagram below. The target type `AppKit-E8-U85` or `SSE-320-U85` selects the board layer for hardware or the simulation model. Using a different AI layer would change the ML model behavior.

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
execute<br/>▼                 | Layer AI       | Executes the ML model (the `execute` method is part of ExecuTorch).
postprocess                   |  Project       | Source file `sds_algorithm_user.cpp` calls `postprocess()` to print the results.

## Target Hardware

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

### Use Training Data

Annotated datasets prepared earlier are used as the training dataset.

The dataset is automatically divided into:

- Training dataset
- Validation dataset

### Pick a Model

In the **Pick a Model** tab, users select a base model from the **ModelNova Cloud Model Registry**.

Users can:

- Browse available models
- Review model details
- Select one model for the workspace

Only **one base model** can be selected per workspace.

The selected model provides the architecture used for training.

  ![Pick a Model](./image/Pick_Model.png)

### Train Your Model

The **Train Your Model** tab is where the AI model is created and optimized.

This section explains how the AI model is built using ModelNova Fusion Studio.

  ![Train Your Model](./image/Train_Model.png)

Users can configure training parameters such as:

- Epoch count
- Batch size
- Learning rate
- Optimizer

Fusion Studio provides training controls including:

- Start training
- Pause training
- Resume training
- Checkpoint recovery

Training progress and logs can be monitored during execution.

### Create Optimized Model for Edge Devices

After training is completed, the model is prepared for deployment to edge hardware.

For ALIF devices, the model must be optimized for:

- **Cortex-M microcontrollers**
- **Ethos-U NPU acceleration**

This ensures efficient execution on low-power embedded devices.

### View Training Report

The **Validate Model** tab evaluates the trained model using the validation dataset.

The validation dataset is automatically generated during dataset splitting.

Fusion Studio provides several evaluation visualizations including:

- Accuracy metrics
- Confusion matrices
- Detection outputs
- Segmentation results
- Prediction comparisons

These visualizations help users understand model performance before deployment.

These reports help evaluate model performance.

  ![validate Your Model](./image/Validate_Model.png)

### Obtain the AI Model

After training and validation, the final trained model becomes available for:

- Deployment
- Compilation
- Inference testing

This model represents the final AI artifact generated by the workspace.

### Deploy Model

The **Deploy Model** tab is used to deploy trained models to supported edge devices.

Fusion Studio currently supports:

- Raspberry Pi
- ALIF devices

Deployment behavior depends on the device type.

  ![Deploy Model](./image/Deploy_Model.png)

### ALIF Device Deployment

ALIF devices require **model compilation before deployment**.

#### Model Compilation

The trained model must be converted into a runtime format compatible with ALIF hardware.

The major step in this process is:

**PTH → PTE conversion**

Where:

- **PTH** = PyTorch trained model format
- **PTE** = optimized runtime format required for ALIF devices

This conversion optimizes the model for:

- Cortex-M processors
- Ethos-U NPU acceleration

After compilation:

1. The optimized model package is generated
2. The compiled model is deployed to the ALIF device
3. The device begins performing inference


### Device Management System in Fusion Studio

The app will support the devices in the list as said for that connection. Other device information can be seen inside the **Device Hub** page. To access this page, click on the **Manage Devices** button on the app home page (which is the workspace list page), located near the **New Workspace** button.

  ![Manage Devices](./image/Manage_Device.png)

Inside the Device Hub, you can see all the device information. In the top right corner, there is an **Add Device** button. Clicking on this opens the device adding page, where you must provide the details based on the device you are connecting.

  ![Add Device](./image/Add_Device.png)

  ![Device Setup](./image/Device_Setup.png)

  ![Device Setup](./image/Device_Setup_2.png)


### Inference Monitoring

Once deployed, the device begins performing **real-time inference**.

Fusion Studio allows users to:

- Monitor predictions
- View inference outputs
- Analyze inference performance

Inference results are streamed directly from the device to the application.

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

> [!NOTE]
> pyOCD can be used with a CMSIS-DAP adapter to capture RTT data file. The application is instrumented with SystemView markers used to measure timing and log messages for additional information. The instrumented application is currently available in the [`sysview`](https://github.com/Arm-Examples/ModelNova/tree/sysview) branch of the project.

Once timing information is collected, it can viewed using the SEGGER [SystemView](https://www.segger.com/products/development-tools/systemview/) tool.

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

This section explains how additional training data and AI model output data are captured using the SDS framework.

Fusion Studio supports capturing new datasets directly from connected devices.

This option appears alongside **Download Dataset** and **Import Dataset** inside the **Data Setup** tab.

Using this feature, users can capture new data from:

- Cameras
- Connected edge devices

The capture system uses the **SDS framework**, which enables:

- Live data capture
- Storage of captured samples
- Recording real-world inference outputs

Captured samples are automatically stored in the workspace dataset repository.

This allows users to:

- Expand the dataset with real-world inputs
- Collect difficult edge cases
- Improve model robustness with real operational data

Captured data can then be annotated and used for future model training.

  ![Capture From Device](./image/Capture.png)

### Annotate Your Data

The **Annotate Your Data** tab is used to label datasets for machine learning.

All datasets collected through:

- Download
- Import
- Capture

will appear in this section.

Annotations depend on the selected ML domain.

![Annotate Your Data](./image/Annotate.png)

After annotation is complete, the dataset becomes the **training dataset**.

## Retrain ML model

**This section explains how the AI model is retrained using additional training data.**

Retraining occurs within the **Train Your Model** stage.

When additional data is collected through the **Capture New Data** feature or imported datasets, users can retrain the model to improve performance.

Retraining workflow:

1. Capture or import additional data
2. Annotate the new dataset
3. Add the dataset to the existing training dataset
4. Resume or restart training

Fusion Studio supports **checkpoint-based training**, allowing the model to resume training from the last checkpoint instead of starting from scratch.

Retraining helps improve:

- Model accuracy
- Detection of difficult cases
- Performance on real-world data

## Regression Test

Regression testing verifies that an updated ML model still matches a fixed reference dataset before deployment. In this setup, recorded inputs (`ML_In.*.sds`) are replayed through SDSIO and the results are compared against the reference outputs (`ML_Out.*.sds`) within a defined tolerance. Run this after every retrain to catch accuracy regressions from architecture, training data, or quantization changes.

The build type `DebugPlay` configures the application for SDS data playback and includes only a subset of the application as outlined in the picture below.

![Regression Test - Data Playback](./image/Regression_Test_Playback.png "Regression Test - Data Playback")

There are two possible target types for SDS data playback:

- Target type `SSE-320-U85` runs on the Corstone-320 FVP simulation model and verifies the correctness of the operation. Simulation models are easy to deploy and do not require any hardware.

- Target type `AppKit-E8-U85` runs on the Alif AppKit-E8 target hardware and uses pyOCD with a CMSIS-DAP unit for hardware-in-the-loop (HIL) testing. Besides correctness, timing can also be verified by capturing an RTT file for the SEGGER SystemView tool.

### Test on `SSE-320-U85`

The branch `simulator` in this repository contains the setup for the FVP simulation model. Select the target `SSE-320-U85` with build type `DebugPlay` and build the project for playback mode. Then run the FVP simulator using the Run action. This uses the file `ML_In.0.sds` to validate the algorithm.

```bash
*  Executing task: FVP_Corstone_SSE-320 -f Board/Corstone-320/fvp_config.txt -a out/AlgorithmTest/SSE-320-U85/DebugPlay/AlgorithmTest.axf  

Ethos-U version info:
        Arch:       v2.0.0
        MACs/cc:    256
        Cmd stream: v1
SDS I/O VSI interface initialized successfully
Model buffer loaded, has 1 methods
Running method forward
Setup Method allocator pool. Size: 1048576 bytes.
Setting up planned buffer 0, size 752640
Method 'forward' loaded.
Model initialized. Ready for inference.
97% idle
99% idle
SDS playback and recording (#0) started
SDS playback and recording (#0) stopped
====

61% idle
99% idle
No more SDS data files for playback of input data!

Info: /OSCI/SystemC: Simulation stopped by user.
```

This run generates the output file `ML_Out.0.sds`, which should be identical to `ML_Out.0.sds.ref` generated with the actual hardware target.

> [!NOTE]
> The simulation is a convenient, hardware-free way to validate correctness; it runs about a factor of 100 slower than real-time execution on hardware due to the detailed Ethos-U NPU modeling.
