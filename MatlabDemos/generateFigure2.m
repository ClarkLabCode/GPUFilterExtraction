clear all;
input = randn(5e7,1,'single');

filterLength = 128;
t = 0:filterLength-1;

cellAFilter =  (t.*exp(-t/30)/(30*exp(-1))).^4;
cellBFilter = -(t.*exp(-t/60)/(60*exp(-1))).^8;

cellAResponse = conv(input,cellAFilter);
cellAResponse = cellAResponse(1:length(input));
cellBResponse = conv(input,cellBFilter);
cellBResponse = cellBResponse(1:length(input));

cellCFiringRateHz = cellAResponse.*(cellAResponse>0) + cellBResponse.*(cellBResponse>0);
numSpikesTrace = poissrnd(cellCFiringRateHz/1000); % Sample bins are 1ms

response = numSpikesTrace;

C = extract2ndOrderKernelGPU(128,input,response);
S = extract2ndOrderKernelGPU(128,input,ones(size(response),'single'));
%K = extract2ndOrderKernelGPU(128,input,response-mean(response))/(2*(std(input)^4)*((1/1000)^2));
C_0 = ((length(input) - 128)/sum(response))*C - S;

STA = simpleSTA(input,numSpikesTrace,128)';
A = STA'*STA;
C_1 = C_0 - A;

APrime = A/(STA*STA');
C_2 = (eye(128)-APrime)*C_0*transpose(eye(128) - APrime);

mats = cat(3,C_0,C_1,C_2);
matNames = {'C_0','C_1','C_2'};
numMats = size(mats,3);

subplot(7,numMats,0*numMats+1);
plot(1:length(cellAFilter'),cellAFilter','m');

subplot(7,numMats,1*numMats+1);
plot(1:length(cellBFilter'),cellBFilter','g');

subplot(7,numMats,1*numMats);
plot(STA);

subplot(7,numMats,2*numMats);
imagesc(A);

for ii = 1:numMats
    
    thisMat = mats(:,:,ii);
    
    subplot(7,numMats,2*numMats+ii);
    imagesc(thisMat);
        
    subplot(7,numMats,3*numMats+ii);
    [eVecs, eVals] = eig(thisMat);
    eigenvalues = sum(eVals);
    [eigenvalues, idxs] = sort(eigenvalues,'descend');
    eVecs = eVecs(:,idxs);
    switch ii
        case 1 %C_0
            selectedIdxs = [1,2];
        case 2 %C_1
            selectedIdxs = [1,128];
        otherwise %C_2
            selectedIdxs = [1];
    end 
    if length(selectedIdxs) == 2
        eVecsSelected = eVecs(:,selectedIdxs);
    else
        eVecsSelected = [STA' eVecs(:,selectedIdxs)];
    end
    plot(1:128,eigenvalues,'.','Color',[0.6 0.6 0.6]);
    hold on;
    plot(selectedIdxs,eigenvalues(selectedIdxs),'r.');
    hold off;

    subplot(7,numMats,4*numMats+ii);
    plot(bsxfun(@rdivide,eVecsSelected,max(abs(eVecsSelected))))    

    subplot(7,numMats,5*numMats+ii);
    filterWeights = eVecsSelected\[cellAFilter' cellBFilter'];
    plot(1:128,cellAFilter','m')
    hold on;
    plot(1:128,eVecsSelected*filterWeights(:,1),'Color',[0.3 0.3 0.3]);
    hold off;
    
    subplot(7,numMats,6*numMats+ii);
    plot(1:128,cellBFilter','g')
    hold on;
    plot(1:128,eVecsSelected*filterWeights(:,2),'Color',[0.3 0.3 0.3]);
    hold off;    
end