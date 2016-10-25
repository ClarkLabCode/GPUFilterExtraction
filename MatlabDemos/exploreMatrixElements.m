clear all;

% This script can aid in the understanding of the STC matrix format. The
% script will generate random stimuli with 8 spatial dimensions. The script
% will then simulate a model cell which responds to a pair of stimuli, as
% chosen by the variable "spatialCorrelation". The Volterra kernel which is
% encoded has 128 time lags and the weights resemble a fly when viewed in
% the standard format. The fly image is only 64 time lags across, but it
% the starting position can be shifted using the "startingTimelags"
% variable.

spatialCorrelation = [1 1];
startingTimelags = [40 10];

inputs = randn(2^14,8,'single');
% Load the image, roatate it, and scale it between -1 and 1
image = single(rot90(imresize(imread('fly.png'),0.5),2))*(2/255) - 1;
% Convert the image from color to greyscale using the NTSC standard weights
image = .299*image(:,:,1)+.587*image(:,:,2)+.114*image(:,:,3);

imageEmbedded = single(zeros(128));
stl = startingTimelags;
imageEmbedded(stl(1):stl(1)+63,stl(2):stl(2)+63) = image;

response(length(inputs),1) = single(0);
for ii = 128:length(inputs)
    inputASnip = inputs(ii-127:ii,spatialCorrelation(1));
    inputBSnip = inputs(ii-127:ii,spatialCorrelation(2));
    corrMatrix = inputASnip*inputBSnip';
    response(ii) = dot(corrMatrix(:),imageEmbedded(:));
end

covMatGPU = extract2ndOrderKernelGPU(128,inputs,response-mean(response));

figure();
imagesc(covMatGPU/2);
axis('equal');
axis('tight');
caxis([-1 1]);