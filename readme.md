# GPU-Accelerated Wiener Filter/STC Matrix Extraction

This program makes use of your GPU to extract the Wiener Filter or STC matrix ([1](https://www.ncbi.nlm.nih.gov/pubmed/27477016),[2](http://www.cns.nyu.edu/pub/lcv/simoncelli03c-preprint.pdf)) from a set of inputs and responses. This can speed up computations >100x compared to commonly used CPU-based implementations. This repository contains a MATLAB wrapper and an example C++ application.

## Install
To use the MATLAB wrapper, you will need the appropriate .mex file for your system. This can be downloaded from this repository's [releases page](https://github.com/ClarkLabCode/GPUFilterExtraction/releases), or you can compile your own using the steps below. If you choose to download a release, follow the steps in the included readme file.

##### Compile

1. Make sure you have a compiler installed such as [Visual Studio](https://www.visualstudio.com/vs/community/) on Windows or [XCode](https://itunes.apple.com/us/app/xcode/id497799835?mt=12) on macOS
2. On Windows, download and install the OpenCL SDK from your GPU vendor.
  * For Nvidia, use the [CUDA SDK](https://developer.nvidia.com/cuda-downloads)
  * For AMD, use the [AMD APP SDK](http://developer.amd.com/tools-and-sdks/opencl-zone/amd-accelerated-parallel-processing-app-sdk/)
3. In MATLAB, navigate to the mex folder in this repository, and type ```edit compile.m```
4. Uncomment the lines pertaining to your OS/GPU vendor, and run the script
  * On macOS, you may need to follow the instructions [here](https://www.mathworks.com/matlabcentral/answers/243868#answer_192936) to enable mex compilation
5. Copy all the files from the mex folder *except* ```compile.m``` and ```extract2ndOrderKernelGPU.cpp``` into your MATLAB path (e.g. ```C:\Users\USERNAME\Documents\MATLAB``` or ```$home/Documents/MATLAB```)
6. Copy the files in the clFiles directory into your MATLAB path. **These files must be placed in the same directory as** ```getClFilePath.m```

## Usage

To recreate figures from our upcoming paper, follow these steps:
1. In MATLAB, navigate to the CreatePaperFigures folder
2. Run ```generateFigure1B.m```
  * The first time you run this script, test data will be generated, which will take a few seconds
3. Run ```generateFigure1CDEF.m``` **Note that this can take several hours to run, depending on your CPU**
  * As an alternative, run ```simpleBenchmark.m``` to get a sense of the speedup on your machine.
  
For more information regarding the MATLAB wrapper function ```extract2ndOrderKernelGPU```, type ```help extract2ndOrderKernelGPU``` in MATLAB.

For an example of how to use the function in C++ (also useful for creating your own wrapper), see ```exampleUsage.cpp```. Note that the output array will contain ```numTau * numSpatial * numTau * numSpatial * numROIs``` elements. These will be organized in the standard STC/Wiener kernel format, such that the response of cell ```c``` to a product of stimulus dimension ```i``` with stimulus dimension ```j``` at timelags ```t1``` and ```t2```, respectively, is stored in the element
```
t1 +
i *numTau + 
t2*numTau*numSpatial + 
j *numTau*numTau*numSpatial +
c *numTau*numTau*numSpatial*numSpatial
```
