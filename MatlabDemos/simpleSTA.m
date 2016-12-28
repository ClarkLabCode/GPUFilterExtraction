function output = simpleSTA(input,response,timeLags)
    input(1:timeLags-1) = 0;
    output = zeros(timeLags,1);
    
    output(1) = response' * input;
    
    for i = 1:timeLags-1
        output(i+1) = response' * [zeros(i,1);input(1:end-i)];
    end
    
    output = output/sum(response);
end