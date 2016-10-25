clear all;
dataFilename = [fileparts(mfilename('fullpath')) filesep() 'data.mat'];
if exist(dataFilename,'file')
    load(dataFilename);
else
    genData;
    clear all;
    load([fileparts(mfilename('fullpath')) filesep() 'data.mat']);
end

image = single(imresize(imread('fly.png'),0.5))*(2/255) - 1;
% Convert the image from color to greyscale using the NTSC standard weights
image = .299*image(:,:,1)+.587*image(:,:,2)+.114*image(:,:,3);

covMatGPU = extract2ndOrderKernelGPU(64,inputs(:,1),response-mean(response));

figure();

subplot(1,3,1);
imagesc(image);
axis('equal');
axis('tight');
caxis([-1 1]);
xlabel('G^{(2)}')

subplot(1,3,2);
imagesc(covMatGPU/2);
axis('equal');
axis('tight');
caxis([-1 1]);
xlabel('K^{(2)}')

subplot(1,3,3);
imagesc(image-covMatGPU/2);
axis('equal');
axis('tight');
caxis([-1 1]);
xlabel('G^{(2)} - K^{(2)}')