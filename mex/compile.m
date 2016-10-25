%% AMD:
% oclPath = getenv('AMDAPPSDKROOT');
% mex('-largeArrayDims','-lopencl',['-L' oclPath 'lib\x86_64'],['-I' oclPath 'include'],'extract2ndOrderKernelGPU.cpp','..\extractKernels.cpp','..\divideNumTauBlocks.cpp','..\OpenCLErrorStrings.cpp','COMPFLAGS="$COMPFLAGS /MT"')

%% Nvidia:
cudaPath = getenv('CUDA_PATH');
mex('-largeArrayDims','-lopencl',['-L' cudaPath '\lib\x64'],['-I' cudaPath '\include'],'extract2ndOrderKernelGPU.cpp','..\extractKernels.cpp','..\divideNumTauBlocks.cpp','..\OpenCLErrorStrings.cpp','COMPFLAGS="$COMPFLAGS /MT"')

%% Mac:
% mex -largeArrayDims extract2ndOrderKernelGPU.cpp ../extractKernels.cpp ../divideNumTauBlocks.cpp ../OpenCLErrorStrings.cpp LDFLAGS="\$LDFLAGS -framework OpenCL"