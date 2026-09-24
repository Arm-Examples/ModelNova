## Model Conversion Details

### Selected Operators

**Source:** Model file: `rps_alif_model.pth` (int8 RPS SqueezeNet Classification)

**Count:** 31 NPU operators (100.0% delegated; CPU operators = 0)

The generated export did not emit a separate selected-operator list. The Vela
summary confirms full NPU delegation, and the SqueezeNet image-classification
graph uses the following operator families:

```text
Conv2D, MaxPool, Clamp, MemoryCopy, AvgPool
```

**Ethos-U Compile Specification:**
  - target: `ethos-u85-256`
  - system_config: `ETHOS_U85_SRAM_MRAM`
  - memory_mode: `SHARED_SRAM`
  - accelerator configuration: `Ethos_U85_256`
  - ethos-u-vela: `5.1.0`
  - extra_flags: `--optimise=Size`

**Quantization Configuration:**
  - Model type: int8 input/output RPS SqueezeNet Classification
  - Quantization flow: post-training int8 quantization for Ethos-U delegate
  - Calibration set: 996 images
  - Input/output quantization folded into the delegate
  - On-device input formula:

```text
int8_input = clip(round(float_input / input_scale) + input_zero_point, -128, 127)
float_output = (int8_output - output_zero_point) * output_scale
```

**I/O Quantization Parameters:**
  - Input type: `int8`
  - Input scale: `0.018649335950613022`
  - Input zero_point: `-14`
  - Output type: `int8`
  - Output scale: `4.433094024658203`
  - Output zero_point: `-128`

**Model Architecture:**
  - Model class: SqueezeNet 1.1 (RPS Classification)
  - Model alias: `squeezenet-classification`
  - Category: Image Classification
  - Input: `f32[1, 3, 224, 224]` before int8 delegate quantization
  - Output: `int8[1, 4]` logits/scores after delegate execution
  - Classes: `PAPER`, `ROCK`, `SCISSOR`, `UNKNOWN`
  - Classifier: convolution classifier at `classifier[1]`
  - Key layers: Conv2D (3x3, 1x1), Fire modules with squeeze/expand, MaxPool, AvgPool

**Vela Compilation Summary:**
  - Accelerator configuration               Ethos_U85_256
  - System configuration              ETHOS_U85_SRAM_MRAM
  - Memory mode                               SHARED_SRAM
  - Accelerator clock                                 400 MHz
  - Design peak SRAM bandwidth                      11.92 GB/s
  - Design peak Off-chip Flash bandwidth             0.72 GB/s
  - Total SRAM used                                924.02 KiB
  - Total Off-chip Flash used                      747.64 KiB
  - CPU operators = 0 (0.0%)
  - NPU operators = 31 (100.0%)
  - Average SRAM bandwidth                           3.18 GB/s
  - Input   SRAM bandwidth                          12.41 MB/batch
  - Weight  SRAM bandwidth                           4.16 MB/batch
  - Output  SRAM bandwidth                           3.38 MB/batch
  - Total   SRAM bandwidth                          20.29 MB/batch
  - Total   SRAM bandwidth            per input     20.29 MB/inference (batch size 1)
  - Average Off-chip Flash bandwidth                 0.12 GB/s
  - Input   Off-chip Flash bandwidth                 0.00 MB/batch
  - Weight  Off-chip Flash bandwidth                 0.76 MB/batch
  - Output  Off-chip Flash bandwidth                 0.00 MB/batch
  - Total   Off-chip Flash bandwidth                 0.76 MB/batch
  - Total   Off-chip Flash bandwidth  per input      0.76 MB/inference (batch size 1)
  - Neural network macs                         266091495 MACs/batch

**Generated Artifacts:**
  - `model.pte`: 785,216 bytes
  - `model_pte.h`: generated C header for the ExecuTorch model
  - `model_config.h`: int8 quantization and label configuration
