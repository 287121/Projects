# Calculator App with ImGui

This project is a simple calculator application built using [Dear ImGui](https://github.com/ocornut/imgui).

## Overview

This repository includes:
- The core ImGui source files and backends.
- A sample calculator application that utilizes ImGui for its GUI.
- Project files for building the application on Windows (Visual Studio project files are provided in the `calculator` directory).

## Disclaimer

This project was created by editing the example GUI provided by the authors of [Dear ImGui](https://github.com/ocornut/imgui). The original example, **example_win32_directx11**, serves as the foundation for this calculator app, and modifications were made to test a different use case.

## Prerequisites

To build and run this project, ensure your development environment includes:

- **.NET Framework:**
  - [.NET Framework v3.5](https://www.microsoft.com/pl-pl/download/details.aspx?id=21)
- **DirectX SDK:**
  - [DirectX SDK (June 2010)](https://www.microsoft.com/en-us/download/details.aspx?id=6812)
- **Dear ImGui:**
  - The ImGui library (included in this repository).
- **Additional Backend Dependencies:**
  - This project is based on the DirectX 11 backend. Make sure your system supports DirectX 11 runtime.

## Getting Started

### Building the Application

#### Visual Studio
1. Open the solution or project file located in the `calculator` directory.
2. Ensure include paths and backend configurations are correct (they are pre-configured for a basic DirectX 11 setup).
3. Build and run the project.
