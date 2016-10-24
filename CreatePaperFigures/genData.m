clear all;

inputs = randn(2^20,2,'single');
image = single(rot90(imresize(imread('fly.png'),0.5),2))*(2/255) - 1;
image = .299*image(:,:,1)+.587*image(:,:,1)+.114*image(:,:,3);

response(length(inputs),1) = single(0);
for ii = 64:length(inputs)
    inputSnip = inputs(ii-63:ii,1);
    corrMatrix = inputSnip*inputSnip';
    response(ii) = dot(corrMatrix(:),image(:));
end

filename = [fileparts(mfilename('fullpath')) filesep() 'data.mat'];
save(filename,'inputs','response')