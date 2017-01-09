# GPU-Accelerated Wiener Filter/STC Matrix Extraction

This program makes use of your GPU to extract the Wiener Filter or STC matrix ([1](https://www.ncbi.nlm.nih.gov/pubmed/27477016),[2](http://www.cns.nyu.edu/pub/lcv/simoncelli03c-preprint.pdf)) from a set of inputs and responses. This can speed up computations >100x compared to commonly used CPU-based implementations. This repository contains a MATLAB wrapper and an example C++ application.

Our paper documenting this algorithm is:

[**Graphics Processing Unit-Accelerated Code for Computing Second-Order Wiener Kernels and Spike-Triggered Covariance**](http://journals.plos.org/plosone/article?id=10.1371/journal.pone.0169842) by Omer Mano and Damon A. Clark

## Compatibility
This program should be compatible with most modern systems and has been sucessfully tested on the following systems:
* OS X El Capitan with Intel Iris 6100 graphics
* macOS Sierra with Intel Iris 6100 graphics
* Windows 7 with Nvidia 1060 graphics
* Windows 7 with Nvidia 560 graphics
* Windows 8 with AMD Fury X graphics
* Windows 10 with Intel HD 510 graphics
* Windows 10 with Nvidia 970m graphics

We have documented problems running on Macs using Intel HD 5000-series integrated graphics (2014 models and earlier). We believe these are due to non-conforming OpenCL drivers (a known issue on Macs).

## Install
To use the MATLAB wrapper, you will need the appropriate .mex file for your system. This can be downloaded from this repository's [releases page](https://github.com/ClarkLabCode/GPUFilterExtraction/releases). **This is the best strategy in most cases**. Follow the steps in the release's included readme file. If you want to compile the code from scratch, use the steps below.

## Compile
**Note that this should not be necessary for most users.** Instead, use our latest [release](https://github.com/ClarkLabCode/GPUFilterExtraction/releases)

1. Make sure you have a compiler installed such as [Visual Studio](https://www.visualstudio.com/vs/community/) on Windows or [XCode](https://itunes.apple.com/us/app/xcode/id497799835?mt=12) on macOS
2. On Windows, download and install the OpenCL SDK from your GPU vendor. MacOS has these libraries built in.
  * For Nvidia, use the [CUDA SDK](https://developer.nvidia.com/cuda-downloads)
  * For AMD, use the [AMD APP SDK](http://developer.amd.com/tools-and-sdks/opencl-zone/amd-accelerated-parallel-processing-app-sdk/)
3. In MATLAB, navigate to the mex folder in this repository, and type ```edit compile.m```
4. Uncomment the lines pertaining to your OS/GPU vendor, and run the script
  * On macOS, you may need to follow the instructions [here](https://www.mathworks.com/matlabcentral/answers/243868#answer_192936) to enable mex compilation
5. Copy all the files from the mex folder *except* ```compile.m``` and ```extract2ndOrderKernelGPU.cpp``` to a convenient location- we'll call this the "install directory".
6. Copy the files in the clFiles directory into your MATLAB path. **These files must be placed in the same directory as** ```getClFilePath.m```
7. Add your install directory to your MATLAB path: navigate to the install directory in MATLAB and type ```addpath(pwd)``` in the terminal.

## Usage

To recreate figures from our upcoming paper, follow these steps:

1. Make sure your install directory is in your MATLAB path (see step 7 in "Compile" or the ```readme.txt``` in the Release directory)
2. In MATLAB, navigate to the MatlabDemos folder
3. Run ```generateFigure1B.m```
  * The first time you run this script, test data will be generated, which will take a few seconds
4. Run ```generateFigure1CDEF.m``` **Note that this can take several hours to run, depending on your CPU**
  * As an alternative, run ```simpleBenchmark.m``` to get a sense of the speedup on your machine.
  
For more information regarding the MATLAB wrapper function ```extract2ndOrderKernelGPU```, type ```help extract2ndOrderKernelGPU``` in MATLAB. View and run ```exploreMatrixElements``` to gain intuition about the output of ```extract2ndOrderKernelGPU```.

For an example of how to use the function in C++ (also useful for creating your own wrapper), see ```exampleUsage.cpp```. Note that the output array will contain ```numTau * numSpatial * numTau * numSpatial * numROIs``` elements. These will be organized in the standard STC/Wiener kernel format, such that the response of cell ```c``` to a product of stimulus dimension ```i``` with stimulus dimension ```j``` at timelags ```t1``` and ```t2```, respectively, is stored in the element
```
t1 +
i *numTau + 
t2*numTau*numSpatial + 
j *numTau*numTau*numSpatial +
c *numTau*numTau*numSpatial*numSpatial
```

## Troubleshooting
|  Error message  |           Solution           |
| --------------- | ---------------------------- |
| Invalid MEX-file: The specificied module could not be found | This most likely means OpenCL could not be found on your system. Try reinstalling your graphics drivers.|
| Undefined function or variable extract2ndOrderKernelGPU | Add the folder containing the correct mex file to your MATLAB path (see step 7 in "Compile" or ```readme.txt``` in the Release directory |
| Attempt to execute SCRIPT extract2ndOrderKernelGPU as function | ```extract2ndOrderKernel.m``` is in your MATLAB path, but the mex file is not. Download the mex file from the [releases page](https://github.com/ClarkLabCode/GPUFilterExtraction/releases) or compile using the steps above.
