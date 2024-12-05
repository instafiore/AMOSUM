%%%%% input %%%%%

days(365).
day(1..365).

% workshift(id, name, hours).
workshift(1,"1-morning",7).
workshift(2,"2-afternoon",7).
workshift(3,"3-night",10).
workshift(4,"4-restafternights",0).
workshift(5,"5-rest",0). %called weekend in the document.
workshift(6,"6-holiday",0).

%%%%% encoding %%%%%

% Choose an assignment for each day and for each nurse.
1 <= {assign(N, T, D) : workshift(T,_,_)} <= 1 :- day(D), nurse(N).

% Each nurse works from 1687 to 1692 hours per year.
:- nurse(N), maxHoursPerYear(MAX), #sum{H,D : assign(N,T,D), workshift(T,_,H)} > MAX.
% ub(MAX, (0, nurse(N), maxHoursPerYear(MAX))) :- nurse(N), assign(N,T,D), workshift(T,_,H), maxHoursPerYear(MAX).
% group(assign(N,T,D), H, (D,N), (0, nurse(N), maxHoursPerYear(MAX)), le_eo) :-  nurse(N), assign(N,T,D), workshift(T,_,H), maxHoursPerYear(MAX).


% :- nurse(N), minHoursPerYear(MIN), #sum{H,D : assign(N,T,D), workshift(T,_,H)} < MIN, N > 1.
#amosum{ H : assign(N,T,D), workshift(T,_,H), H > 0 [(D,N)] } >= MIN : nurse(N), minHoursPerYear(MIN).

% lb(MIN, (0, nurse(1), minHoursPerYear(MIN))) :- nurse(1), assign(1,T,D), workshift(T,_,H), minHoursPerYear(MIN).
% group(assign(1,T,D), H, (D,1), (0, nurse(1), minHoursPerYear(MIN)), ge_eo) :-  nurse(1), assign(1,T,D), workshift(T,_,H), minHoursPerYear(MIN).

% lb(MIN, (0, nurse(2), minHoursPerYear(MIN))) :- nurse(2), assign(2,T,D), workshift(T,_,H), minHoursPerYear(MIN).
% group(assign(2,T,D), H, (D,2), (0, nurse(2), minHoursPerYear(MIN)), ge_eo) :-  nurse(2), assign(2,T,D), workshift(T,_,H), minHoursPerYear(MIN).

% lb(MIN, (0, nurse(3), minHoursPerYear(MIN))) :- nurse(3), assign(3,T,D), workshift(T,_,H), minHoursPerYear(MIN).
% group(assign(3,T,D), H, (D,3), (0, nurse(3), minHoursPerYear(MIN)), ge_eo) :-  nurse(3), assign(3,T,D), workshift(T,_,H), minHoursPerYear(MIN).

%id_aux(nurse(N), minHoursPerYear(MIN)) :- nurse(N), minHoursPerYear(MIN).
%group(id_aux(nurse(N), minHoursPerYear(MIN)), MIN, (nurse(N), minHoursPerYear(MIN)), (0, nurse(N), minHoursPerYear(MIN)), ge_amo) :- id_aux(nurse(N), minHoursPerYear(MIN)).

% Each nurse cannot work twice in 24 hours.
:- nurse(N), assign(N, T1, D), assign(N, T2, D+1), T2 < T1, T2 <= 3, T1 <= 3.

% Exactly (or at least, to check) 30 days of holidays.
%:- nurse(N), #count{D:assign(N,6,D)} < 30.
:- nurse(N), #count{D:assign(N,6,D)} != 30.

% After two consecutive nights there is one rest day.
:- not assign(N,4,D), assign(N,3,D-2), assign(N,3,D-1).
:- assign(N,4,D), not assign(N,3,D-2).
:- assign(N,4,D), not assign(N,3,D-1).

% At least 2 rest days each 14 days.
:- nurse(N), day(D), days(DAYS), D <= DAYS-13, #count{D1:assign(N,5,D1), D1>=D, D1 < D+14} < 2.

% Each morning the number of nurses can range from 6 to 9.
:- day(D), #count{N:assign(N,1,D)} > K, maxNurseMorning(K).
:- day(D), #count{N:assign(N,1,D)} < K, minNurseMorning(K).

% Each afternoon the number of nurses can range from 6 to 9.
:- day(D), #count{N:assign(N,2,D)} > K, maxNurseAfternoon(K).
:- day(D), #count{N:assign(N,2,D)} < K, minNurseAfternoon(K).

% Each night the number of nurses can range from 4 to 7.
:- day(D), #count{N:assign(N,3,D)} > K, maxNurseNight(K).
:- day(D), #count{N:assign(N,3,D)} < K, minNurseNight(K).

% Fair distribution (morning, afternoon, night)
% ---> morning
:- nurse(N), #count{D : assign(N,1,D)} > MAXDAYS, maxDays(MAXDAYS).
:- nurse(N), #count{D : assign(N,1,D)} < MINDAYS, minDays(MINDAYS).
% ---> afternoon
:- nurse(N), #count{D : assign(N,2,D)} > MAXDAYS, maxDays(MAXDAYS).
:- nurse(N), #count{D : assign(N,2,D)} < MINDAYS, minDays(MINDAYS).
% ---> night
:- nurse(N), #count{D : assign(N,3,D)} > MAXNIGHTS, maxNights(MAXNIGHTS).
:- nurse(N), #count{D : assign(N,3,D)} < MINNIGHTS, minNights(MINNIGHTS).

% #show assign/3.
