# Christmas Clock

## Gitpod

When starting in gitpod, the environment is automatically prepared. All required packages are installed and the raspberry pi pico sdk is cloned from the git master branch.

## SDK Setup

Install the following packages to cross compile for the RP2040:

```bash
sudo apt install cmake build-essential gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib
```

### Clone the C/C++ Pico SDK

best to do this in your home directory

```bash
git clone https://github.com/raspberrypi/pico-sdk.git --recursive
```

## Prepare build files (Pico SDK from git)

By default builds for RP2040 (Pico). To build for **RP2350 (Pico 2)**, add `-DPICO_BOARD=pico2`.

```bash
mkdir build
cd build

# RP2040 (default)
cmake -DPICO_SDK_FETCH_FROM_GIT=ON ..

# RP2350
cmake -DPICO_SDK_FETCH_FROM_GIT=ON -DPICO_BOARD=pico2 ..

make -j4
```

## Prepare build files (local Pico SDK)

If the SDK is located in ~/pico-sdk the script start-env can be used:

```bash
. start-env
```

If the file github-id contains the private key used for your github account the script will start a seperate ssh-agent with only that key.

Otherwise use:

```bash
mkdir build
cd build

# RP2040 (default)
cmake -DPICO_SDK_PATH=/path/to/pico-sdk ..

# RP2350
cmake -DPICO_SDK_PATH=/path/to/pico-sdk -DPICO_BOARD=pico2 ..
```

# Set flash memory size

If your board has more flash memory than the default 2MB, you can specify the flash size in bytes using the `PICO_FLASH_SIZE_BYTES` variable. For example, if your rp2350 board has 16MB of flash memory, you can set it like this:

```bash
cmake -DPICO_SDK_PATH=/path/to/pico-sdk/ -DPICO_BOARD=pico2 -DPICO_FLASH_SIZE_BYTES=16777216 ..
```

## Build

```bash
make -j$(nproc)
```

## Deploy

- Press BOOTSEL button on the Pico board and reset the board
- Copy the `build/ChristmasClock.uf2` file to the Pico board

## Testing

Unit tests use [Google Test](https://github.com/google/googletest) and run on the host (no hardware required). App logic is compiled against mock Pico SDK headers so everything builds with your regular host compiler.

### Build tests

```bash
mkdir build-test
cd build-test
cmake -DBUILD_TESTS=ON ..
make -j$(nproc)
```

### Run all tests

```bash
./test/test_all
```

### Run individual test suites

```bash
./test/test_utils
./test/test_variable_store
./test/test_spfs
./test/test_dot_matrix
```

### Filter specific tests

Google Test supports `--gtest_filter` for running a subset:

```bash
./test/test_all --gtest_filter='DotMatrixTest.*'
./test/test_dot_matrix --gtest_filter='DotMatrix8xNTest.TwoModules_ScrollHi'
```
