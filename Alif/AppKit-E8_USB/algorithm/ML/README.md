# ML Layer

This directory contains the Machine Learning layer for Object Detection using the YOLO Fastest ML model. The ML layer provides the necessary components and models to perform face detection on embedded devices with or without Arm Ethos-U NPU acceleration.

## Components

### CMSIS Libraries

The layer integrates industry-standard CMSIS libraries for efficient ML operations:

- **CMSIS-DSP**: Digital signal processing optimizations
- **CMSIS-NN**: Neural network kernels optimized for Cortex-M processors

### TensorFlow Lite Micro

Complete TensorFlow Lite Micro integration including:

- **Kernel Variants**: 
  - CMSIS-NN kernels for devices without Ethos-U
  - Ethos-U optimized kernels for NPU acceleration
- **Supporting Libraries**: flatbuffers, gemmlowp, kissfft, ruy
- **ML Eval Kit**: API, logging, math utilities, and object detection algorithms

### Image Processing Functions

The image processing component (`image_processing_func.c/h`) provides essential pre-processing utilities for preparing camera input for ML inference:

- **Debayering**: Converts raw Bayer-pattern images from camera sensors into full RGB format
  - Supports all standard Bayer patterns (RGGB, BGGR, GRBG, GBRG)
  - Handles optional red/blue channel swapping
  - Uses efficient bilinear interpolation

- **Image Resizing**: Scales images to match the ML model input requirements
  - Supports multiple output formats: grayscale, RGB565, RGB888
  - Uses bilinear interpolation for high-quality results
  - Optimized for embedded systems

- **Format Conversion**: Transforms images between different color formats
  - RGB888 to RGB565 conversion
  - RGB to grayscale conversion
  - Handles channel swapping when needed

> **NOTE**
>
> - Provided image processing functions serve only for example purposes.
>   Optimized library shall be used in production ready application.

## Models

### YOLO-Fastest v4 Face Detection Model

The layer includes multiple variants of the YOLO-Fastest 192x192 face detection model optimized for different hardware configurations:

#### 1. **Ethos-U55 (256 MACs)** - `yolo-fastest_192_face_v4_vela_H256.tflite.cpp`
- Optimized for devices with Arm Ethos-U55 NPU (256 MACs per cycle)
- Uses Vela compiler optimizations
- Best suited for Cortex-M55 processors with Ethos-U55

#### 2. **Ethos-U65 (256 MACs)** - `yolo-fastest_192_face_v4_vela_Y256.tflite.cpp`
- Optimized for devices with Arm Ethos-U65 NPU (256 MACs per cycle)
- Uses Vela compiler optimizations
- Higher performance variant for Cortex-M processors with Ethos-U65

#### 3. **Ethos-U85 (256 MACs)** - `yolo-fastest_192_face_v4_vela_Z256.tflite.cpp`
- Optimized for devices with Arm Ethos-U85 NPU (256 MACs per cycle)
- Uses Vela compiler optimizations
- Latest generation NPU support

#### 4. **CPU-Only** - `yolo-fastest_192_face_v4.tflite.cpp`
- Runs on Cortex-M processors without NPU acceleration
- Uses CMSIS-NN optimized kernels
- Suitable for devices without Ethos-U NPU

### Model Capabilities

All model variants provide:

- **Input**: 192x192 pixel images (RGB or grayscale)
- **Detection Task**: Real-time face detection
- **Output**: Bounding boxes with confidence scores for detected faces
- **Architecture**: YOLO-Fastest v4 - ultra-lightweight object detection network
- **Use Cases**: 
  - Face detection for security applications
  - People counting and presence detection
  - Smart camera and IoT applications
  - Human-machine interface

## Usage

### Target Type Specification

The ML layer needs properly specified build context to automatically select the appropriate model variant.

Based on the target hardware configuration the target-type shall use the naming convention:

```yml
<name>-<NPU family>-<NPU MACs>
# Where:
# - <name> can be arbitrary name
# - <NPU family> is the Ethos-U NPU family name (U55, U65, U85), if omitted Ethos-U NPU won't be used
# - <NPU MACs> is the number of NPU MACs (128, 256), if omitted 256 MACs is assumed
```

Examples
```yml
target-types:
  - type: myDevice-U55-128 # Specifies Ethos-U55 with 128 MACs
    ...
  - type: myBoard-U65      # Specifies Ethos-U65 with 256 MACs
    ...
```

### Image Processing

For image pre-processing, use the functions in `image_processing_func.h` to convert camera data to the required format before feeding it to the ML model.

## Dependencies

- CMSIS-DSP v1.16.0+
- CMSIS-NN v7.0.0+
- ML Embedded Eval Kit v24.8.0+
- TensorFlow Lite Micro v1.25.2+
