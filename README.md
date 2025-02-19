# Signal Analysis README

## Overview
This project consists of two main functions:
1. `saveSignal`: Reads oscilloscope data from a CSV file, processes it, and stores it in a tree in a ROOT file.
2. `histosMaking`: Loads the processed data from the ROOT file and generates histograms for visualization.

## Dependencies
The code requires the following libraries:
- ROOT (https://root.cern/)
- C++ Standard Library
- `mySignal_cxx.so`

## Usage
### Compiling and Running
Ensure that the `mySignal_cxx.so` is loaded in the ROOT session you're working in. You can do it by typing:
```
gSystem->Load("mySignal_cxx.so")
```

### `saveSignal` Function
This function reads data from a CSV file and saves processed signals in a ROOT file.

### `histosMaking` Function
This function reads the ROOT file created by `saveSignal` and generates histograms for analysis.

## Output
1. `saveSignal` generates a ROOT file named `[fileID]signalSaved.root`.
2. `histosMaking` produces histograms stored in `outfile.root`.
3. Drawn histograms:
   - `t0h`: Start time distribution
   - `TOTh`: Time-over-threshold distribution
   - `Qh`: Charge distribution
   - `aMaxh`: Maximum amplitude distribution
   - `aMaxQh`: 2D histogram of amplitude vs charge
   - `Qt0h`: 2D histogram of charge vs start time

## Notes
- Adjust histogram binning and ranges as needed for different datasets.
- Set `debug = kTRUE` inside `saveSignal` for additional debug output.