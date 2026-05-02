# DONKUS PROJECT

Fast and simple multimedia compression for the unwashed masses.

## Building

### Windows MinGW

Building:
```
cmake -B build -G "MinGW Makefiles"
cmake --build build
```

Testing:
```
ctest --test-dir build --output-on-failure
```