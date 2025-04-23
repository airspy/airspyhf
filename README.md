# User mode driver for Airspy HF+ 

This repository contains host software (Linux/Windows) for Airspy HF+, a high performance software defined radio for the HF and VHF bands.

http://www.airspy.com/airspy-hf-plus

## How to build host software on Windows:

### For VisualStudio 2013 or later:

* `git clone https://github.com/airspy/airspyhf.git host`
* Download https://github.com/libusb/libusb/releases/download/v1.0.20/libusb-1.0.20.7z
* Extract **libusb-1.0.20.7z** to host directory
  * You should have **host\libusb-1.0.20**
* Download ftp://mirrors.kernel.org/sourceware/pthreads-win32/pthreads-w32-2-9-1-release.zip
* Extract **pthreads-w32-2-9-1-release.zip** to host directory
  * You should have **host\libpthread-2-9-1-win**
* Navigate to **src** and Launch **airspyhf.sln** with VisualStudio 2013 or later
* In Visual Studio, choose **Release**, **x86** or **x64** then **Build Solution**

### For MinGW:

`git clone https://github.com/airspy/airspyhf.git host`

`cd host`

`mkdir build`

`cd build`

Normal version:

`cmake ../ -G "MSYS Makefiles" -DLIBUSB_INCLUDE_DIR=/usr/local/include/libusb-1.0/`

Debug version:

`cmake ../ -G "MSYS Makefiles" -DCMAKE_BUILD_TYPE=Debug -DLIBUSB_INCLUDE_DIR=/usr/local/include/libusb-1.0/`

`make`

`make install`


## How to build the host software on Linux:

### Prerequisites for Linux (Debian/Ubuntu/Raspbian):


`sudo apt-get install build-essential cmake libusb-1.0-0-dev pkg-config`


### Build host software on Linux:

`wget https://github.com/airspy/airspyhf/archive/master.zip`

`unzip master.zip`

`cd airspyhf-master`

`mkdir build`

`cd build`

`cmake ../ -DINSTALL_UDEV_RULES=ON` or
`cmake ../ -USE_UACCESS_RULES=ON` or
`cmake ../` (see usage notes below)

`make`

`sudo make install`

`sudo ldconfig`

Note: The default installation is designed for networked systems, such as SpyServers, that require an extra level of security to access their device. As such, using the cmake option `-DINSTALL_UDEV_RULES=ON` will allow read/write permissions only for the logged in user and those that are included in the `plugdev` group. If this default MODE/GROUP paradigm is employed, the `plugdev` group will be automatically created during the installation phase. However, it is up to the system admistrator to ensure that all local and/or remote users are subsequently added to that group.

Conversely, Users of stand-alone non-Debian-based Linux systems may require less stringent `uaccess` udev rules in order for applications to 'see' the device. By using the cmake option `-DUSE_UACCESS_RULES=ON` the build process will dynamically change and install the udev rules such that the device is created using the `uaccess` paradigm, instead of the default MODE/GROUP.

This can later be reversed to use the default MODE/GROUP paradigm, if needed, by rebuilding with the `-DUSE_UACCESS_RULES=OFF -DINSTALL_UDEV_RULES=ON` options to restore and re-install it.

MacOS deals with communicating with USB devices much differently than Linux. Therefore none of these udev options are required when running `cmake` on Macs.

## Clean CMake temporary files/dirs:

`cd airspyhf-master/build`

`rm -rf *`


## Principal authors:

Ian Gilmour <ian@sdrsharp.com> and Youssef Touil <youssef@airspy.com> 


http://www.airspy.com

This file is part of Airspy HF (with user mode driver based on Airspy R2, itself based on HackRF project see http://greatscottgadgets.com/hackrf/).
