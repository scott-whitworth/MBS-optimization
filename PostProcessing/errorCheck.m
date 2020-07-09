function [] = errorCheck(seed)

    filename = join(['errorCheck-',num2str(seed),'.bin']);
    file = fopen(filename);
    A = fread(file,[3,Inf],'double');
    
    t = A(1,:);
    W = A(2,:);
    dE = A(3,:);

    dW = zeros(1,size(W,2));
    dE = zeros(1,size(E,2));
    err = zeros(1,size(t,2));
    
    for i = 2:size(t,2)
        dW(i) = W(i) - W(i-1);
        dE(i) = E(i) - E(i-1);
        err(i) = (dW(i) - dE(i))./E(i);
    end
    
    figure(1)
    plot(t, err)
    xlabel('t')
    ylabel('% error')