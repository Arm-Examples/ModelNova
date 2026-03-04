
## 🔄 Model Conversion Details

### Selected Operators

**Source:** Model file: alif_sensor_25epochs.pth (SqueezeNet RPS Classification)

**Count:** 48 NPU operators (excluding CPU quantize/dequantize wrapper)

```
Conv2D, MaxPool, Clamp, MemoryCopy, AvgPool
quantized_decomposed::dequantize_per_tensor.out (wrapper)
quantized_decomposed::quantize_per_tensor.out (wrapper)
```

**Ethos-U Compile Specification:**
  - target: ethos-u85-256
  - system_config: Ethos_U85_SRAM_MRAM
  - memory_mode: Shared_Sram
  - extra_flags: --output-format=raw, --debug-force-regor, --verbose-all

**Quantization Configuration:**
  - Using EthosUQuantizer with symmetric quantization
  - Post-training quantization enabled

**Model Architecture:**
  - Model class: SqueezeNet (RPS Classification)
  - Input: f32[1, 3, 224, 224] (RGB images, 224x224)
  - Output: f32[1, 4] (4-class classifier: Rock, Paper, Scissors, Background)
  - Key layers: Conv2D (3x3, 1x1), Fire modules with squeeze/expand, MaxPool, AvgPool

**Vela Compilation Summary:**
  - Accelerator configuration               Ethos_U85_256
  - System configuration             Ethos_U85_SRAM_MRAM
  - Memory mode                               Shared_Sram
  - Accelerator clock                                 400 MHz
  - Design peak SRAM bandwidth                      11.92 GB/s
  - Design peak Off-chip Flash bandwidth             0.72 GB/s
  - Total SRAM used                                960.27 KiB
  - Total Off-chip Flash used                     747.66 KiB
  - CPU operators = 0 (0.0%)
  - NPU operators = 48 (100.0%)
  - Average SRAM bandwidth                           1.67 GB/s
  - Input   SRAM bandwidth                          11.68 MB/batch
  - Weight  SRAM bandwidth                           4.18 MB/batch
  - Output  SRAM bandwidth                           5.06 MB/batch
  - Total   SRAM bandwidth                          21.26 MB/batch
  - Total   SRAM bandwidth            per input     21.26 MB/inference (batch size 1)
  - Average Off-chip Flash bandwidth                 0.06 GB/s
  - Input   Off-chip Flash bandwidth                 0.00 MB/batch
  - Weight  Off-chip Flash bandwidth                 0.73 MB/batch
  - Output  Off-chip Flash bandwidth                 0.00 MB/batch
  - Total   Off-chip Flash bandwidth                 0.73 MB/batch
  - Total   Off-chip Flash bandwidth  per input      0.73 MB/inference (batch size 1)
  - Original Weights Size                          704.69 KiB
  - NPU Encoded Weights Size                       718.75 KiB
  - Neural network macs                        267693188 MACs/batch
  - Info: The numbers below are internal compiler estimates.
  - For performance numbers the compiled network should be run on an FVP Model or FPGA.
  - Network Tops/s                                   0.04 Tops/s
  - NPU cycles                                   5075829 cycles/batch
  - SRAM Access cycles                            674460 cycles/batch
  - DRAM Access cycles                                  0 cycles/batch
  - On-chip Flash Access cycles                         0 cycles/batch
  - Off-chip Flash Access cycles                  389938 cycles/batch
  - Total cycles                                  5083373 cycles/batch
  - Batch Inference time                12.71 ms, 78.69 inferences/s (batch size 1)
