function [] = thrustProgress(str)
% thrustProgress: plots generational thruster data from a binary file
    % Input:
        % str: the specific generation to be plotted ('Best' or 'Worst')
    % Output:
        % N/A: see figures

fin = fopen(join([str,'ThrustGens.bin']));
A = fread(fin, [16 Inf], 'double');

% Gamma graphs
figure(1)
title('Gamma over generations')
subplot(2,4,1)
plot(A(1,:),A(2,:))
xlabel('generations'), ylabel('\gamma_0 (rad.)')
subplot(2,4,2)
plot(A(1,:),A(3,:))
xlabel('generations'), ylabel('\gamma_1 (rad.)')
subplot(2,4,3)
plot(A(1,:),A(4,:))
xlabel('generations'), ylabel('\gamma_2 (rad.)')
subplot(2,4,4)
plot(A(1,:),A(5,:))
xlabel('generations'), ylabel('\gamma_3 (rad.)')
subplot(2,4,5)
plot(A(1,:),A(6,:))
xlabel('generations'), ylabel('\gamma_4 (rad.)')
subplot(2,4,6)
plot(A(1,:),A(7,:))
xlabel('generations'), ylabel('\gamma_5 (rad.)')
subplot(2,4,7)
plot(A(1,:),A(8,:))
xlabel('generations'), ylabel('\gamma_6 (rad.)')

% Tau graphs
figure(2)
title('Tau over generations')
subplot(2,2,1)
plot(A(1,:),A(9,:))
xlabel('generations'), ylabel('\tau_0 (rad.)')
subplot(2,2,2)
plot(A(1,:),A(10,:))
xlabel('generations'), ylabel('\tau_1 (rad.)')
subplot(2,2,3)
plot(A(1,:),A(11,:))
xlabel('generations'), ylabel('\tau_2 (rad.)')

% Coast graphs
figure(3)
title('Coast over generations')
subplot(2,3,1)
plot(A(1,:),A(12,:))
xlabel('generations'), ylabel('coast_0')
subplot(2,3,2)
plot(A(1,:),A(13,:))
xlabel('generations'), ylabel('coast_1')
subplot(2,3,3)
plot(A(1,:),A(14,:))
xlabel('generations'), ylabel('coast_2')
subplot(2,3,4)
plot(A(1,:),A(15,:))
xlabel('generations'), ylabel('coast_3')
subplot(2,3,5)
plot(A(1,:),A(16,:))
xlabel('generations'), ylabel('coast_4')

fclose(fin);