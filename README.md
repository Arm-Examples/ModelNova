**Work in Progress**

![ModelNova Fusion Studio](./Documentation/ModelNova.png "ModelNova Fusion Studio")

This repository explains how **Edge AI applications with Cortex-M/Ethos-U based microcontrollers** are created. It uses [ModelNova](https://modelnova.ai/) for the AI model development and [Keil MDK](https://www.keil.arm.com/) for the embedded development.

## Learn more at Embedded World 2026

### Arm: Hall 4-504

Application Development, Data Capturing, and Workload Analysis of an Edge AI system based on [Alif E8](https://www.keil.arm.com/packs/ensemble-alifsemiconductor).

### ModelNova: Hall 4-600

AI Model Selection, Data Labelling, and Model Creation for an Edge AI system based on [Alif E8](https://www.keil.arm.com/packs/ensemble-alifsemiconductor).

### Exhibitor forum: Hall 5-210 on Wed, 03/11/2026, 09:30 - 10:00

[From model to market: accelerated AI development on Arm](https://www.embedded-world.de/en/conferences-programme/2026/exhibitor-forum/from-model-to-market-accelerated-ai-development): don't miss this talk as it explains the overall AI model development workflow.

## AI model development

The diagram below shows the overall development of an AI model that is integrated into an embedded system. During AI model development the [SDS framework](https://www.keil.arm.com/packs/sds-arm/overview/) is used as a workbench for data capturing and system analysis. Once the optimized model delivers the expected performance it can be integrated into the final application that may be based on FreeRTOS, Keil RTX, or Zephyr.

![AI model development workflow](https://github.com/ARM-software/SDS-Framework/blob/main/overview/MLOps.png "AI model development workflow")

### Initial development steps

1. [Create the input interface](./Documentation/Input_Interface.md)  with signal conditioning and start capturing data for AI model training.
2. [Select an AI model](.), used the captured data for training, analysis, and creation of the optimized AI model.
3. [Integrate the AI model](./Documentation/Integrate_AI_model.md) into the SDS framework and analyze performance.

### Refine AI model development

1. [Capture new data](./Documentation/Capture_Data.md) where the AI model does not deliver the expected results.
2. [Retrain the AI model](./Documentation/Train_AI_model.md) using additional training data to optimize performance.
3. [Add regression testing](./Documentation/Regression_Test.md) before integrating a new AI model into the embedded system.

## Quick Start

### Keil MDK

1. Install [Keil Studio for VS Code](https://marketplace.visualstudio.com/items?itemName=Arm.keil-studio-pack) from the VS Code marketplace.
2. Clone this repository (for example using [Git in VS Code](https://code.visualstudio.com/docs/sourcecontrol/intro-to-git)) or download the ZIP file. Then open the base folder in VS Code.
3. Open the [CMSIS View](https://mdk-packs.github.io/vscode-cmsis-solution-docs/userinterface.html#2-main-area-of-the-cmsis-view) in VS Code and use the ... menu to choose an example via *Select Active Solution from workspace*.
4. The related tools and software packs are downloaded and installed. Review progress with *View - Output - CMSIS Solution*.
5. In the CMSIS view, use the [Action buttons](https://github.com/ARM-software/vscode-cmsis-csolution?tab=readme-ov-file#action-buttons) to build, load and debug the example on the hardware.

### ModelNova Fusion Studio

1. Download and install [ModelNova Fusion Studio](https://modelnova.ai/fusion-studio-beta).
2. ...

## Issues or Questions

Use the [**Issues**](./issues) tab to raise questions or issues.
