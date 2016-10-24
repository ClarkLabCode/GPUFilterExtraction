% Adapted from https://github.com/pillowlab/iSTAC
function covMatCPU = extract2ndOrderKernelCPU(numTemporal,inputsSelected,responseSelected)
    [respLength, numSpatial, numROIs] = size(inputsSelected);
    covMatCPU = zeros(numSpatial*numTemporal,numSpatial*numTemporal,numROIs);
    inputExpanded = zeros(length(responseSelected),numSpatial*numTemporal);
    responseSelected(1:(numTemporal-1),:) = 0;
    for roi = 1:numROIs
        for ii = 1:numSpatial
            inputExpanded(:,1+(ii-1)*numTemporal:ii*numTemporal) = toeplitz(inputsSelected(:,ii,roi),[inputsSelected(1,ii,roi) zeros(1,numTemporal-1)]);
        end
        covMatCPU(:,:,roi) = inputExpanded'*(inputExpanded.*repmat(responseSelected(:,:,roi),[1 numSpatial*numTemporal]));
    end
    covMatCPU = covMatCPU/(respLength-(numTemporal-1));
end