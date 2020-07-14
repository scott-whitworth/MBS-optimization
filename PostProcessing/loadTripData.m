function [tripTime,coast_threshold,y0E,y0A,gammaCoeff,tauCoeff,coast] = loadTripData(cVector)

%% Array offsets

        %% config offsets
        asteroid_offset = 1; % x(1:6) y0A
        earth_offset = 7; % x(7:12) y0E
        threshold_offset = 13; % x(13) coast threshold

        % array sizes
        gamma_size = cVector(14);
        tau_size = cVector(15);
        coast_size = cVector(16);
        
        GAMMA_OFFSET = 17; % x(17:16+gamma_size) third order fourier for in-plane angle
        TAU_OFFSET = GAMMA_OFFSET + gamma_size; % x(17+gamma_size:16+gamma_size+tau_size) first order fourier for out-of-plane angle
        TRIPTIME_OFFSET = TAU_OFFSET + tau_size + 3; % x(17+gamma_size+tau_size) total duration of the trip (tau coeffs + alpha, beta, zeta)
        COAST_OFFSET = TRIPTIME_OFFSET + 1; % x(18+gamma_size+tau_size) second order fourier for coasting determination
        
        %% Constants
        
        tripTime=cVector(TRIPTIME_OFFSET);
        coast_threshold = cVector(threshold_offset);
        
         %% Initial conditions of Earth and Asteroid
        
        % Asteroid
        % y0A = [1.03524021423705E+00, 1.59070192235231E-01, -5.54192740243213E-02,...
        % -2.53512430430384E-08, 2.27305994342265E-07, 7.29854417815369E-09];
        y0A = cVector(asteroid_offset:earth_offset-1);

        % Earth
        % y0E = [1.00140803662733E+00, 1.2786132931868E-01, -1.195365359889E-05,...
        % -3.30528017791942E-09, 1.98791889005860E-07, -9.89458740916469E-12];
        y0E = cVector(earth_offset:threshold_offset-1);
        
        %% Initial Fourier components
        
        gammaCoeff= cVector(GAMMA_OFFSET:GAMMA_OFFSET-1+gamma_size);
        tauCoeff=cVector(TAU_OFFSET:TAU_OFFSET-1+tau_size);
        coast=cVector(COAST_OFFSET:COAST_OFFSET-1+coast_size);
end

