RTL-Spec
========

# Installation
The following installation instructions are currently only tested for Debian-based Linux operating systems (such as Ubuntu or Raspbian).

## Dependencies
### libusb
libusb is a C library that gives applications easy access to USB devices on many different operating systems. RTL-Spec uses the library to interface the RTL-SDR USB dongle. More details to libusb are available at http://www.libusb.org.
```sh
$ sudo apt-get install libusb-1.0-0-dev
```

### librtlsdr
librtlsdr is a C library that turns RTL2832 based DVB-T dongles into SDR receivers. More details to librtlsdr are available at http://sdr.osmocom.org.
```sh
$ git clone git://git.osmocom.org/rtl-sdr.git
$ cd rtl-sdr/
$ mkdir build && cd build/
$ cmake ../ -DINSTALL_UDEV_RULES=ON
$ make
$ sudo su
$ make install
$ ldconfig
$ cat > /etc/modprobe.d/rtl-blacklist.conf << EOL
$ blacklist dvb_usb_rtl28xxu
$ blacklist rtl2832
$ blacklist rtl2830
$ EOL
$ rmmod dvb_usb_rtl28xxu rtl2832
$ exit
$ cd ../../
```

### fftw
fftw is a C library for computing the discrete Fourier transform (DFT). More details to fftw are available at http://www.fftw.org.
```sh
$ sudo apt-get install fftw-dev
```

## RTL-Spec
#### Cloning
```sh
$ git clone https://github.com/pdamian/rtl-spec.git
```
#### Building
```sh
$ cd rtl-spec/
$ make collector CFLAGS="-O2 -DVERBOSE"
$ make sensor_cpu CFLAGS="-O2 -DVERBOSE"
```
    make <TARGET> [CFLAGS="<CFLAGS>"]
    <TARGET> = sensor_cpu | sensor_gpu | collector
    <CFLAGS> = [-O2] [-ggdb] [-DVERBOSE] [...]
    
#### Running
```sh
$ ./run_collector 5000
$ ./run_cpu_sensor 24000000 1766000000
```

#### Description
Multiple sensing nodes can be served by a remote collecting unit. The RF spectrum data recorded by sensors is transmitted over TCP to an associated collector which dumps the received data to the local file system. The following figure highlights the involved processing steps:

![alt text](https://github.com/pdamian/rtl-spec/blob/master/images/processing_steps.png "Processing Steps")

The dumped data is stored in the following format:

| Seconds since UNIX Epoch [secs] | Timestamp Extension [microsecs] | Frequency [Hz] | Squared Magnitude Value (dB) |
| ------------------------------- | ------------------------------- | -------------- | ---------------------------- |
| 1423490796                      | 854275                          | 23996876       | -33.9                        |
| 1423490796                      | 854275                          | 24006250       | -20.6                        |
| ...                             | ...                             | ...            | ...                          |
