% This benchmark is an easier task that should only take a few minutes to
% complete. It may be an underestimate of GPU performance on larger tasks
dataFilename = [fileparts(mfilename('fullpath')) filesep() 'data.mat'];
if exist(dataFilename,'file')
    load(dataFilename);
else
	disp('Generating test data');
    genData;
    clear all;
    load([fileparts(mfilename('fullpath')) filesep() 'data.mat']);
end


numROIs = 32;
inputLength = 2^17;
numSpatial = 4;
numTemporal = 64;

% Select correct length from response and mean subtract
responseSelected = repmat(bsxfun(@minus,response(1:inputLength),mean(response(1:inputLength))),[1 1 numROIs]);
% inputs array has only two spatial dimensions. spatialIdxs allows us to
% repeat those dimensions the appropriate number of times
spatialIdxs = mod((1:numSpatial)-1,2)+1;
inputsSelected = repmat(bsxfun(@minus,inputs(1:inputLength,spatialIdxs),mean(inputs(1:inputLength,spatialIdxs))),[1 1 numROIs]);

disp('Running on CPU: this could take a couple minutes.');
% CPU Timing
tic
covMatCPU = extract2ndOrderKernelCPU(numTemporal,inputsSelected,responseSelected);
cpuTime = toc;

disp('Running on GPU');
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
set(gca,'XTick',[]);set(gca,'YTick',[]);
colorbar;

subplot(1,3,2);
imagesc(covMatGPU(:,:,1));
axis('equal');axis('tight');
xlabel('GPU');
set(gca,'XTick',[]);set(gca,'YTick',[]);
colorbar;

subplot(1,3,3);
imagesc(covMatCPU(:,:,1) - covMatGPU(:,:,1));
axis('equal');axis('tight');
xlabel('CPU - GPU');
set(gca,'XTick',[]);set(gca,'YTick',[]);
colorbar;