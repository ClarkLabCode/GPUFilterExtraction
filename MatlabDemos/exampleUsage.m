function exampleUsage()
    respLength = 10000;
    numTau = 64;
    numROIs = 1;
    numSpatial = 1;
        
    % Set Up Input Data
    stimulus = randn(respLength,numSpatial,numROIs,'single');
    % Correlate the first input to itself at an offset of (1,2)
    response = circshift(stimulus(:,1,:),1,1).*circshift(stimulus(:,1,:),2,1);
    
    output = extract2ndOrderKernelGPU(numTau,stimulus,response);
    imagesc(output(:,:,1));
end