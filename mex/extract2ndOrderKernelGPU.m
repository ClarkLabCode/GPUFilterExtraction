%   EXTRACT2NDORDERKERNELGPU use a Graphics Processing Unit to estimate
%   Wiener kernels
%
%   Kernels = EXTRACT2NDORDERKERNELGPU(numTau,Stimuli,Responses) where
%   numTau is a scalar, Stimuli is a T x D x C matrix and Responses is a 
%   T x 1 x C matrix, returns the second order Wiener kernel of the
%   responses to the stimuli for numTau timelags.
%
%   T is the length of the stimuli in samples. D is the number of 
%   independent stimuli or stimulus dimensions presented to each measured
%   cell. C is the number of responding cells. Kernels is a
%   numTau*D x numTau*D x C matrix which represents the Wiener kernel 
%   estimate of C responding cells.
%
%   Note: Further dimensions beyond C in the inputs will be treated as
%   additional independent cells, and the structure will be preserved in
%   Kernels. For instance, if Stimuli is a T x D x C x S matrix
%   representing S shuffles of the data and Responses is a T x 1 x C x S
%   matrix, Kernels will be a numTau*D x numTau*D x C x S matrix.
%
%   The layout of the Kernels matrix follows the standard STC/Wiener kernel
%   matrix format, such that the response to a product of stimulus
%   dimension i with stimulus dimension j at timelags t1 and t2,
%   respectively, is stored in the element
%   ((i-1)*numTau + t1,(j-1)*numTau + t2). For examples, see
%   exampleUsage.m, generateFigure1B.m, or generateFigures1CF.m