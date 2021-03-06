%MINIMIZETRIPTIME Searches for the lowest converging trip time guess

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%This function is designed to take an order for a fourier
%fit, an estimate of how long the trip will take and then a convergance
%radius for its solution. Given that information it will optimize
%trajectories for the ship to land on the asteroid for successively smaller
%time guesses until it finds the minimum possible time in which the ship
%can successfully land. That value is then returned as the most efficient
%time. Much of the relevant problem information specifically about the
%ship's pecifications and asteroid orbital parameters are read in from
%other files. Almost all functionality of the program branches off of this
%function.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%Called by MultipleRuns
function [finalTime,escapeTime,timeToExecute,phiCoeff,plotFileName,rError,thetaError,uError,hError,fval,lastConverge,massPowerPlant,massThruster,massStruct,mdot,lowestEscape,lowestConverge,lowestReturnConverge,lowestForwardConverge,refinedTimeStep,FuelMass,lowestdepartBound,lowestapproachBound,lowestcoastFraction] = MinimizeTripTimeFast(FS,order,direction,massFuel,numberOfEngines,alphaP,alphaT,inputPower,uExhaust,efficiency,massPayload,massStruct,massSample,lastConverge,selectForwardAltitude,selectReturnAltitude,entryVelocity,entryAngle,astDes,timeWait,isSolar,powerLevel,thrusterName,numPoints,thrusterStructs,maxTimeLimit,COAST,departBound,approachBound,numIntervals,coastFraction,LegNumber,flagStore,optimize,interpolateStruct,throttleFlag)

%starts timer for function
tic;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%Non-dimensionalize and initialize constants

%Non-dimensionalize constants
earthRadius = Constants.EARTHR / Constants.MCONVERSION;
forwardAltitude = selectForwardAltitude / Constants.MCONVERSION;
returnAltitude = selectReturnAltitude / Constants.MCONVERSION;
entryVelocity = entryVelocity * (Constants.SCONVERSION/Constants.MCONVERSION);
entryAngle = entryAngle * Constants.DEGTORADCONVERSION;
%Initialize variables

%Non-dimensional starting altitude of launch measured from earth's center
forwardRadius = earthRadius + forwardAltitude;

%Non-dimensional ending altitude of spacecraft measured from earth's center
returnRadius = earthRadius + returnAltitude;

%Initialize mass of the power plant and the thrusters
massPowerPlant = numberOfEngines * alphaP * inputPower;
massThruster = numberOfEngines * alphaT * inputPower;

%Initializes final values of r, theta, and u for Earth and Asteroid, and h
%of Asteroid based on text files stating the relative positions of earth
%and asteroid. It calculates the point of closest approach and sets the
%conditions of closest approach as the conditions at landing.
[earthCloseApp,asteroidCloseApp,AsteroidSemimajor,AsteroidEccentricity] = GetFinalState(astDes);

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%The forward trip also carries the fuel for the return trip as calculated
%in MultipleRuns.
massPayload = massPayload + massFuel;

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
%fminsearch is an unconstrained optamization package in MATLAB.  It gets
%passed a vector of independent variables, in this case 1 variable
%(timeGuess) and a function to evaluate each guess with. The
%maximum number of function evaluations and maximum iterations must not be
%set too high because fminsearch has a tendency to get trapped at random
%time guesses and will iterate infinitely. In almost every case, fminsearch
%will produce an F value in less than 20,000 iterations, or it will iterate
%infinitely. The tolerance on the independent variable (TolX) and the
%tolerance on the function evaluation value (TolFun) in this circumstance
%will be very close to identical.  If the ship successfully lands on the
%asteroid in the time allotted, then timeObjectiveFunction will return the
%timeGuess as the function value. If it fails, timeObjectiveFunction
%will return a very large value.  As long as the ship is landing on the
%asteroid, the variation in X and the objective function will be identical,
%Therefore, they are assigned to the same value.

%A drawback of using fminsearch like this on a 1 dimensional function is
%that it knows nothing about the search space.  We know that the next
%lowest answer will always be at a lower time guess than the previous but
%there is no way to input that information to the algorithm. For some time
%we used a binary search but from time to time there are irregularities in
%the results from timeObjectiveFunction and fminsearch is much more capable
%of handling those than our binary search was.

%As a precurser to the main time optimizing search algorhythm, we wrote an
%algorhythm to generate a good starting time guess from which the main
%algorhythm begins.  This is accomplished by beginning at the lowest
%feasible time that a trip could take and stepping at a large time interval
%(~0.1 yr) forward through time until the value returned by fminsearch for
%the value of the function minimum dips below a fairly high threshold
%(~1E-3). Once such a value is found, the algorithm steps backward at a
%smaller time interval (~0.05 yr) until it finds two instances of values
%for the function returned by fminsearch which are above the threshold. The
%time at which this occurs is stored as the value for which the main
%time optimization algorhythm will begin.

lastfval = nan;
lastEscape = nan;

%Setting the max and min time guesses
largeForwardStep = rand(1) * .01 + 0.05; %interval for large forward steps
lowerTimeLimit = rand(1)*.1 + .6; %lowest expectedw limit of trip time
upperTimeLimit = 20; %highest expected limit of trip time
if direction == Direction.RETURN
    lastConverge = nan;
    maxTimeGuess = 20;
elseif direction == Direction.FORWARD
    maxTimeGuess = 20;
end


%Initialize lowest values
lowestConverge = maxTimeGuess;
lowestcConverge = (pi/2)*(-1 + 2*rand(1,2*order+3));
lowestfval = maxTimeGuess;
lowestfinalMass = maxTimeGuess;
lowestescapeVelocity = maxTimeGuess;
lowestescapeEarthAngle = maxTimeGuess;
lowestearthConditions = zeros(4,1);
lowestasteroidConditions = zeros(4,1);
lowestdepartBound = maxTimeGuess;
lowestapproachBound = maxTimeGuess;
lowestcoastFraction = maxTimeGuess;
lowestYdata = zeros(5,1);
lowestTdata = zeros(5,1);

if direction == Direction.FORWARD
    lowestEscape = NaN;
else
    lowestEscape = maxTimeGuess;
end
%The following for loop will find the optimal MinTimeGuess for the
%current time function so that the optimization algorithm will run more
%quickly


MaxIterations = Constants.FMIN_MAXNUMITER;

interestingFlag = false;
for timeGuess = lowerTimeLimit : largeForwardStep : upperTimeLimit
    %     Randomize coefficients
    cConverge = (pi/2)*(-1 + 2*rand(1,2*order+3));
    if ~interestingFlag && optimize(LegNumber,1)
        departBound(LegNumber,1) = rand(1) * .3 + .051;
        approachBound(LegNumber,1) = rand(1) * .14 + .051;
        coastFraction(LegNumber,1) = rand(1) * .45 + .51;
    end

    [cConverge,converge,lastConverge,lastfval,lastEscape,mdot,finalMass,escapeVelocity,escapeEarthAngle,earthConditions,asteroidConditions,~,departBound,approachBound,coastFraction,lastYdata,lastTdata] = ...
        timeObjectiveFunction(FS,cConverge,order,direction,timeGuess,timeWait,numberOfEngines,uExhaust,inputPower,efficiency,massPowerPlant,massThruster,massPayload,massStruct,massSample,forwardRadius,returnRadius,entryVelocity,entryAngle,lastConverge,lastfval,lastEscape,earthCloseApp,asteroidCloseApp,isSolar,COAST,departBound,approachBound,numIntervals,coastFraction,LegNumber,MaxIterations,optimize,interpolateStruct,throttleFlag);

    %     Store lowest values if converge and if it is lowest trip time
    
    [lowestConverge,lowestcConverge,lowestEscape,lowestfval,lowestfinalMass,lowestescapeVelocity,lowestescapeEarthAngle,lowestearthConditions,lowestasteroidConditions,lowestdepartBound,lowestapproachBound,lowestcoastFraction,lowestYdata,lowestTdata] =...
        StoreLowestValues(converge,lastConverge,cConverge,lastEscape,lastfval,finalMass,escapeVelocity,escapeEarthAngle,earthConditions,asteroidConditions,departBound,approachBound,coastFraction,lowestConverge,lowestcConverge,lowestEscape,lowestfval,lowestfinalMass,lowestescapeVelocity,lowestescapeEarthAngle,lowestearthConditions,lowestasteroidConditions,lowestdepartBound,lowestapproachBound,lowestcoastFraction,flagStore,lowestYdata,lowestTdata,lastYdata,lastTdata);

    if converge
        break;
    end
end

refinedTimeStep = largeForwardStep;

if direction == Direction.RETURN
    lowestReturnConverge = lowestConverge;
    lowestForwardConverge = nan;
else
    lowestForwardConverge = lowestConverge;
    lowestReturnConverge = nan;
end


%Call IntegrateForPlot to integrate lowest values for plotting
[FuelMass,Ydata,Tdata,Adata,departData,coastData,thrustData,approachData,MdotData,TmdotData] = IntegrateForPlot(COAST,direction,LegNumber,efficiency,mdot,numberOfEngines,inputPower,lowestEscape,lowestfinalMass,numIntervals,interpolateStruct,throttleFlag,lowestYdata,lowestTdata);


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

finalTime = lowestConverge;
escapeTime = lowestEscape;
phiCoeff = lowestcConverge;
fval = lowestfval;

asteroidConditions = lowestasteroidConditions;

%Plot spacecraft motion variables with respect to time
[plotFileName,rError,thetaError,uError,hError] = ...
    PlotAndWriteToFile(order,direction,phiCoeff,fval,escapeTime,finalTime,timeWait,uExhaust,asteroidCloseApp,asteroidConditions,astDes,powerLevel,thrusterName,Tdata,Ydata,thrusterStructs,Adata,departData,coastData,thrustData,approachData,COAST,LegNumber,AsteroidSemimajor,AsteroidEccentricity,MdotData,TmdotData);

%Ends timer for function
timeToExecute = toc;
end
