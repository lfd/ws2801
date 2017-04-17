WS2801 kernelspace driver
=========================

Build & Load
------------

    make
    insmod ws2801.ko

Add device instance
-------------------

### Example device-tree entry
TBD. Explain DT-Overlays.

    led-stripe {
        compatible = "ws2801";

        clk-gpios = <&gpio 12 GPIO_ACTIVE_HIGH>;
        data-gpios = <&gpio 13 GPIO_ACTIVE_HIGH>;
        num-leds = <40>;
        refresh-rate = <5000>;
        /* auto-commit; */
        status = "okay";
    };

sysfs Interface
---------------

All the driver functionality is available through a sysfs interface.  Every
driver instance has an own entry in "/sys/devices/ws2801/devices".  Available
attributes:

### sync
Writes to sync commit pending changes.

Example:

    echo > sync

### num_leds
Reading from it returns the number of LEDs.  Writing to num_leds can be used to change the configuration.

Example:

    echo 50 > num_leds
    cat num_leds

### set
Used to update one or more LEDs. The first LED has the number zero. Multiple LEDs can be set at once.

Example:

    # Sets the eleventh LED to RGB (255, 0, 255)
    echo "10 255 0 255" > set
    echo > sync
    # Sets two LEDs at once
    echo -en "10 255 0 255\n11 255 0 0\n" > set
    echo > sync

### full_on
Sets all LEDs at once to a specified RGB value.

Example:

    echo 255 255 0 > full_on
    echo > sync
    
### clear
Sets all LEDs at once to RGB (0, 0, 0).

Example:

    echo > clear
    echo > sync

### refresh_rate
Refresh rate in milliseconds. If no changes occur, and this timeout is passed, the LEDs will be force updated. Zero-value disables automatic refreshes.

Example:

    echo 0 > refresh_rate
    cat refresh_rate
    echo 100 > refresh_rate

### auto_commit
Automatically commits any change immediately. Not recommended. Might cause unintended effects.

Example:

    echo 1 > auto_commit
    cat auto_commit
    echo 0 > auto_commit
