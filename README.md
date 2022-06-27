# Replacement esphome Dallas component.

This can be used to replace the built in Dallas component that appears to not work well with later versions of platformio for ESP32

It's pretty much a drop in replacement, with the exception being the name. `dallasng`

Underneath it uses the OneWireNg library from https://github.com/pstolarz/OneWireNg

To use it, just reference an external component

```
external_components:
  - source: github://nrandell/dallasng
```

Set up the dallas component and sensor

```
dallasng:
  - pin: 27
    update_interval: 3s

sensor:
  - platform: dallasng
    index: 0
    name: Temperature
    resolution: 12
    expire_after: 1000s
    filters:
      - filter_out: NAN

```

You can either use an index or an address. The index is ideal if you just have a single sensor, but with multiple sensors you are better off using an address.
