% ---------- Facts ----------
% unit(UnitID, MaxWorkload).
% task(TaskID, Duration).
% capable(UnitID, TaskID, Efficiency).
% dependent(TaskID, TaskBeforeID).
% time_slot(SlotID).

% Assign a unit to a task in a time slot if capable
{ assign(U,T,S) : capable(U,T,_) , time_slot(S) } <= 1 :- unit(U,_).

% Transitive closure
dependent(X,Y) :- dependent(X,Z), dependent(Z,Y).

% Do not exceed unit's workload
:- #sum { D,T,S : assign(U,T,S), task(T,D) } > MaxW, unit(U,MaxW).

% Respect task dependencies
:- assign(U1,T1,S1), assign(U2,T2,S2), dependent(T2,T1), S2 <= S1.

% ---------- Objective: maximize total efficiency ----------
#amomaximize { E : assign(U,T,S), capable(U,T,E) [U]}.