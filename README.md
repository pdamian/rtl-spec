RTL-Spec
========

# Installation
The following installation instructions are currently only tested for Debian-based Linux operating systems (such as Ubuntu or Raspbian).

## Dependencies
### libusb
More details to libusb are available at http://www.libusb.org.
```sh
$ sudo apt-get install libusb-1.0-0-dev
```

### librtlsdr
More details to librtlsdr are available at http://sdr.osmocom.org.
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
More details to fftw are available at http://www.fftw.org.
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
$ ./collector.exe 5000
$ ./sensor_cpu.exe 24000000 1766000000
```
