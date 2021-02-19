## mcp2221(A) hidapi C-library

Yet another lib for MCP2221 https://www.microchip.com/wwwproducts/en/MCP2221A

Inspired by:
- https://github.com/nonNoise/PyMCP2221A
- https://github.com/zkemble/libmcp2221

First one is only python, 2nd one I got into memory issues (unstable).  
Therefore this lib, which is a bare minimum variant.


### Installation

### Building

#### MinGW gcc (win10)

Depends on `hidapi` library.  
A copy of https://github.com/libusb/hidapi dll and header file are present in this repo.

```sh
make mcp2221_hidapi.dll
```

Deploy both `dll` files.

#### linux gcc (ubuntu 20)

1. Depends on `hidapi` library

```sh
sudo apt-get install libudev-dev libusb-1.0-0-dev libhidapi-dev
```

2. Set correct udev rules

```sh
 echo 'KERNEL=="hidraw*", ATTRS{idVendor}=="04d8", MODE="0666"' | sudo tee /etc/udev/rules.d/22-microchip.rules >/dev/null
```

3. compile this lib:

```sh
make libmcp2221_hidapi.so
```

Deploy `libmcp2221_hidapi.so`
