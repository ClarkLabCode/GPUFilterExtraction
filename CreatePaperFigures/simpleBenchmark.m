% This benchmark is an easier task that should only take a few minutes to
% complete. It may be an underestimate of GPU performance on larger tasks
dataFilename = [fileparts(mfilename('fullpath')) filesep() 'data.mat'];
if exist(dataFilename,'file')
    load(dataFilename);
else
    genData;
    clear all;
    load([fileparts(mfilename('fullpath')) filesep() 'data.mat']);
end


numROIs = 32;
inputLength = 2^17;
numSpatial = 4;
numTemporal = 64;


responseSelected = repmat(bsxfun(@minus,response(1:inputLength),mean(response(1:inputLength))),[1 1 numROIs]);
spatialIdxs = mod(1:numSpatial,2)+1;
inputsSelected = repmat(bsxfun(@minus,inputs(1:inputLength,spatialIdxs),mean(inputs(1:inputLength,spatialIdxs))),[1 1 numROIs]);

% CPU Timing
tic
covMatCPU = extract2ndOrderKernelCPU(numTemporal,inputsSelected,responseSelected);
cpuTime = toc;

% OCL Timing
tic;
covMatGPU = extract2ndOrderKernelGPU(numTemporal,inputsSelected,responseSelected);
oclTime = toc;

figure();
bar([cpuTime oclTime]');
set(gca,'XTickLabel',{'CPU', 'GPU'})
ylabel('Calculation Time (s)');

figure();
subplot(1,3,1);
imagesc(covMatCPU(:,:,1));
axis('equal');axis('tight');
xlabel('CPU');
axis off;
colorbar;

subplot(1,3,2);
imagesc(covMatGPU(:,:,1));
axis('equal');axis('tight');
xlabel('GPU');
set(gca,'xticklabel',[])
axis off;
colorbar;

subplot(1,3,3);
imagesc(covMatCPU(:,:,1) - covMatGPU(:,:,1));
axis('equal');axis('tight');
xlabel('CPU - GPU');
axis off;
colorbar;