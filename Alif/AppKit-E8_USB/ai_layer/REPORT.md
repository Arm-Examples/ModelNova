# ExecuTorch AI Layer Build Report

**Generated:** 2026-02-17T07:40:23Z
**Git Commit:** unknown on unknown
**Repository Status:** ✅ Clean
**Last Commit:** unknown

## 📊 Build Summary

- **Libraries:** 9 static libraries
- **Models:** 1 model assets
- **Operators:** 2 selected operators
- **Build Type:** Release

## 📚 Library Assets

**Total Size:** 9.9 MiB

| Library | Size | Percentage | Modified | Hash |
|---------|------|------------|----------|------|
| libcortex_m_kernels.a | 20.0 KiB | 0.2% | 2026-02-17 07:39:22 | `f8f2255205e9c628` |
| libcortex_m_ops_lib.a | 12.6 KiB | 0.1% | 2026-02-17 07:39:22 | `c24914175192f67d` |
| libexecutorch.a | 51.9 KiB | 0.5% | 2026-02-17 07:39:22 | `5201c1cba2cd0310` |
| libexecutorch_core.a | 216.5 KiB | 2.1% | 2026-02-17 07:39:22 | `4751f3b3a26f61fd` |
| libexecutorch_delegate_ethos_u.a | 18.6 KiB | 0.2% | 2026-02-17 07:39:22 | `1844784beab1af3b` |
| libportable_kernels.a | 9.1 MiB | 92.3% | 2026-02-17 07:39:22 | `484d2768079ca390` |
| libportable_ops_lib.a | 199.1 KiB | 2.0% | 2026-02-17 07:39:22 | `ecd676a9d24d56b0` |
| libquantized_kernels.a | 238.6 KiB | 2.4% | 2026-02-17 07:39:22 | `69cc79b6c2a22a28` |
| libquantized_ops_lib.a | 28.7 KiB | 0.3% | 2026-02-17 07:39:22 | `4c0a8ab9d1c0a60b` |

## 🤖 Model Assets

| Asset | Type | Size | Modified | Hash |
|-------|------|------|----------|------|
| ethos_u_minimal_example.pte | Model | 3.8 KiB | 2026-02-17 07:40:23 | `bd7a211160a18572` |

## ⚙️ Build Configuration

### CMake Configuration
- **Build Type:** `Release`
- **Toolchain File:** `/workspace2/model/arm-none-eabi-gcc.cmake`
- **ARM Baremetal:** `ON`
- **Cortex-M Support:** `ON`
- **Portable Ops:** `ON`
- **Quantized Kernels:** `ON`

### Selected Operators

**Source:** Model file: ethos_u_minimal_example.pte (inferred)

**Count:** 2 operators

```
quantized_decomposed::dequantize_per_tensor.out
quantized_decomposed::quantize_per_tensor.out
```

## 🔄 Model Conversion Details

**Ethos-U Compile Specification:**
  - target: ethos-u55-128
  - system_config: Ethos_U55_High_End_Embedded
  - memory_mode: Shared_Sram
  - extra_flags: --output-format=raw, --debug-force-regor, --verbose-all

**Quantization Configuration:**
  - Using EthosUQuantizer with symmetric quantization
  - Post-training quantization enabled

**Model Architecture:**
  - Model class: Add

**Vela Compilation Summary:**
  - Accelerator configuration               Ethos_U55_128
  - System configuration             Ethos_U55_High_End_Embedded
  - Memory mode                               Shared_Sram
  - Accelerator clock                                 500 MHz
  - Design peak SRAM bandwidth                       3.73 GB/s
  - Design peak Off-chip Flash bandwidth             0.47 GB/s
  - Total SRAM used                                  0.14 KiB
  - Total Off-chip Flash used                        0.03 KiB
  - CPU operators = 0 (0.0%)
  - NPU operators = 12 (100.0%)
  - Average SRAM bandwidth                           0.27 GB/s
  - Input   SRAM bandwidth                           0.00 MB/batch
  - Weight  SRAM bandwidth                           0.00 MB/batch
  - Output  SRAM bandwidth                           0.00 MB/batch
  - Total   SRAM bandwidth                           0.00 MB/batch
  - Total   SRAM bandwidth            per input      0.00 MB/inference (batch size 1)
  - Average Off-chip Flash bandwidth                 0.04 GB/s
  - Input   Off-chip Flash bandwidth                 0.00 MB/batch
  - Weight  Off-chip Flash bandwidth                 0.00 MB/batch
  - Output  Off-chip Flash bandwidth                 0.00 MB/batch
  - Total   Off-chip Flash bandwidth                 0.00 MB/batch
  - Total   Off-chip Flash bandwidth  per input      0.00 MB/inference (batch size 1)
  - Original Weights Size                            0.00 KiB
  - NPU Encoded Weights Size                         0.00 KiB
  - Neural network macs                                 0 MACs/batch
  - Info: The numbers below are internal compiler estimates.
  - For performance numbers the compiled network should be run on an FVP Model or FPGA.
  - Network Tops/s                                   0.00 Tops/s
  - NPU cycles                                        349 cycles/batch
  - SRAM Access cycles                                 24 cycles/batch
  - DRAM Access cycles                                  0 cycles/batch
  - On-chip Flash Access cycles                         0 cycles/batch
  - Off-chip Flash Access cycles                       32 cycles/batch
  - Total cycles                                      349 cycles/batch
  - Batch Inference time                 0.00 ms, 1432664.76 inferences/s (batch size 1)

## 📦 Source Layer Export

**Total Source Files:** 226 files across 2 layers

| Layer | Description | Groups | Files |
|-------|-------------|--------|-------|
| stage1 | Generated from compile_commands.json (327 files) | 4 | 37 |
| stage2 | Generated from compile_commands.json (240 files) | 1 | 189 |

### Group Details

**STAGE1:**

- `Runtime`: 19 files
- `Schema`: 1 files
- `Kernels/quantized`: 11 files
- `Backends`: 6 files

**STAGE2:**

- `Kernels`: 189 files


## 🛠️ Build Environment

- **Platform:** `Linux c5a6280b58af 6.6.87.2-microsoft-standard-WSL2 #1 SMP PREEMPT_DYNAMIC Thu Jun  5 18:30:46 UTC 2025 x86_64 x86_64 x86_64 GNU/Linux`
- **Python:** `Python 3.12.3`
- **CMake:** `cmake version 4.2.3`
- **ARM GCC:** `arm-none-eabi-gcc (Arm GNU Toolchain 13.3.Rel1 (Build arm-13.24)) 13.3.1 20240614`

## 📁 Asset Locations

```
ai_layer/
├── engine/
│   ├── lib/           # Static libraries
│   ├── include/       # Header files
│   └── model/         # Model assets
└── REPORT.md          # This report
```

---
*Report generated by ExecuTorch AI Layer build system at 2026-02-17T07:40:23Z*