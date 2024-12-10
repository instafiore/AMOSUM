% INSTANCE
% --------------------------------------------------------------------------------------------
person(1..5).
project(ID, BUDGET).
% budget for each project
% role(Per,S)
role(1,"full_professor").
role(2,"full_professor").
role(3,"associate_professor").
role(4,"associate_professor").
role(5,"researcher").

% requires_role(Pro,Sk)
requires_role(1,"full_professor").
requires_role(1,"researcher").
requires_role(2,"full_professor").
requires_role(2,"associate_professor").


project_month(Pro, SM, EM)

% ENCODING
% --------------------------------------------------------------------------------------------
salary("full_professor",70).
salary("associate_professor",50).
salary("researcher",30).
max_hours_working(125).
month(1..12)

project_has_role(Pro, S) :- join_project(Per,Pro,_), role(Per,S).
:- requires_role(Pro, S), not project_has_role(Pro, S).

:- person(Per), month(M), max_hours_working(MAX), #sum{H,Pro: join_project(Per, Pro, H, M)} > MAX.

% amo
{join_project(Per,Pro, H, M) : H=1..MAXH, max_hours_per_project(MAX), max_hours(Per, M, MAXH)} <= 1 :- person(Per), project(Pro), month(M), project_month(Pro, SM, EM), M >= SM, M <= EM.

% sum
:- project(Pro, B), #sum{ H*S, Per, M: join_project(Per, Pro, H, M), role(Per,R), salary(R,S)} < B.

% #progetti 5-10-15
% #persone 30-40-50-60
% ricercatori 15%
% ordinari 10%
% associati 75%
% 3 instanze budget bassi (trivially satisfiable)
% 3 instanze budget poco più alti mps (trivially unsatisfiable AMOSUM)
% 4 instanze budget poco più alti mps (trivially unsatisfiable AMOSUM)

#show join_project/3.





