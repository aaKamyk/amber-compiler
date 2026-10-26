<br />
<div align="center">
  <h3>Amber Compiler</h3>
</div>

<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>

1. [About The Project](#about-the-project)
2. [Prerequisites](#prerequisites)
3. [Dependencies](#dependencies)
4. [Installation](#installation)
5. [Usage](#usage)
6. [License](#license)

</details>

<!-- ABOUT THE PROJECT -->

## About The Project

A compiler implementation for a toy rust-inspired language written in C++, entirely from scratch. The lowest available
target currently is LLVM IR, but the plan is to emit binary code for a target architecture in the future.

<!-- GETTING STARTED -->

### Prerequisites

These are the required programs to build and run this project

* CMake
* A C++ Compiler

### Dependencies

* the fmt C++ library [https://github.com/fmtlib/fmt]()
* the cpp_result C++ library [https://github.com/Jarsop/cpp_result]()

### Installation

1. Clone the repo

   ```sh
   git clone https://github.com/aaKamyk/amber-compiler.git
   ```

2. Create a build folder

   ```sh
   mkdir build && cd build
   ```
3. Run CMake
   ```sh
   cmake .. && cmake --build .
   ```

<!-- USAGE EXAMPLES -->

## Usage

Example command for compiling a source file

   ```sh
   act source.amber -o source.ll
   ```

If no output path is provided ./input_path_name.ll is used

<!-- LICENSE -->

## License

Distributed under the MIT license. See `LICENSE.txt` for more information.