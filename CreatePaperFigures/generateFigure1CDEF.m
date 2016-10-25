% Warning: Due to the slow computations on the CPU, this function can take
% over an hour to complete. For a quick comparison, please use
% simpleBenchmark.m
clear all;
disp('Warning: This script takes over an hour to complete on a high-end machine due to slow CPU extraction.');
dataFilename = [fileparts(mfilename('fullpath')) filesep() 'data.mat'];
if exist(dataFilename,'file')
    load(dataFilename);
else
    genData;
    clear all;
    load([fileparts(mfilename('fullpath')) filesep() 'data.mat']);
end

numROIsVec = [1 4 16 64 256];
inputLengthVec = 2.^(16:20);
numSpatialVec = [1 2 4 8 16];
numTemporalVec = [8 16 32 64 128];

for varToChange = 1:4
    for changeIdx = 1:length(numROIsVec)
            numROIs = numROIsVec(4);
            inputLength = inputLengthVec(4);
            numSpatial = numSpatialVec(4);
            numTemporal = numTemporalVec(4);
        switch varToChange
            case 1
                numROIs = numROIsVec(changeIdx);
            case 2
                inputLength = inputLengthVec(changeIdx);
            case 3
                numSpatial = numSpatialVec(changeIdx);
            case 4
                numTemporal = numTemporalVec(changeIdx);
        end
        responseSelected = repmat(single(bsxfun(@minus,response(1:inputLength),mean(response(1:inputLength)))),[1 1 numROIs]);
		% inputs array has only two spatial dimensions. spatialIdxs allows us to
		% repeat those dimensions the appropriate number of times
        spatialIdxs = mod(1:numSpatial,2)+1;
        inputsSelected = repmat(single(bsxfun(@minus,inputs(1:inputLength,spatialIdxs),mean(inputs(1:inputLength,spatialIdxs)))),[1 1 numROIs]);
                
        % CPU Timing
        tic
        covMatCPU = extract2ndOrderKernelCPU(numTemporal,inputsSelected,responseSelected);
        time = toc;
        cpuTime(varToChange,changeIdx) = time;

        % OCL Timing
        tic;
        covMatGPU = extract2ndOrderKernelGPU(numTemporal,inputsSelected,responseSelected);
        time = toc;
        oclTime(varToChange,changeIdx) = time;
    end
end

save('timing.mat','cpuTime','oclTime');

%% Figure showing calculation time on a loglog plot
figure();
subplot(2,2,1);
loglog(inputLengthVec,cpuTime(2,:),inputLengthVec,oclTime(2,:));
xlabel('Number of Samples');
ylabel('Calculation Time');
axis('tight');
ylim([0.1 2000]);
set(gca,'XTick',inputLengthVec);
set(gca,'XTickLabel',{'2^{16}','2^{17}','2^{18}','2^{19}','2^{20}'});

subplot(2,2,2);
loglog(numSpatialVec,cpuTime(3,:),numSpatialVec,oclTime(3,:));
xlabel('Number of Spatial Dimensions');
ylabel('Calculation Time');
axis('tight');
ylim([0.1 2000]);
set(gca,'XTick',numSpatialVec);

subplot(2,2,3);
loglog(numTemporalVec,cpuTime(4,:),numTemporalVec,oclTime(4,:));
xlabel('Number of Temporal Offsets');
ylabel('Calculation Time');
axis('tight');
ylim([0.1 2000]);
set(gca,'XTick',numTemporalVec);

subplot(2,2,4);
loglog(numROIsVec,cpuTime(1,:),numROIsVec,oclTime(1,:));
xlabel('Number of Responses');
ylabel('Calculation Time');
axis('tight');
ylim([0.1 2000]);
set(gca,'XTick',numROIsVec);

%% Figure showing speedup factor
figure()
subplot(2,2,1);
semilogx(inputLengthVec,cpuTime(2,:)./oclTime(2,:));
xlabel('Number of Samples');
ylabel('Speedup');
axis('tight');
ylim([0 250]);
set(gca,'XTick',inputLengthVec);
set(gca,'XTickLabel',{'2^{16}','2^{17}','2^{18}','2^{19}','2^{20}'});

subplot(2,2,2);
semilogx(numSpatialVec,cpuTime(3,:)./oclTime(3,:));
xlabel('Number of Spatial Dimensions');
ylabel('Speedup');
axis('tight');
ylim([0 250]);
set(gca,'XTick',numSpatialVec);

subplot(2,2,3);
semilogx(numTemporalVec,cpuTime(4,:)./oclTime(4,:));
xlabel('Number of Temporal Offsets');
ylabel('Speedup');
axis('tight');
ylim([0 250]);
set(gca,'XTick',numTemporalVec);

subplot(2,2,4);
semilogx(numROIsVec,cpuTime(1,:)./oclTime(1,:));
xlabel('Number of Responses');
ylabel('Speedup');
axis('tight');
ylim([0 250]);
set(gca,'XTick',numROIsVec);