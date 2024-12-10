% INSTANCE
% --------------------------------------------------------------------------------------------
person(1..5).
project(1..3).
% budget for each project
budget(1000).
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

% ENCODING
% --------------------------------------------------------------------------------------------
salary("full_professor",70).
salary("associate_professor",50).
salary("researcher",30).
max_hours_per_project(10).
max_hours_working(10).

project_has_role(Pro, S) :- join_project(Per,Pro,_), role(Per,S).
:- requires_role(Pro, S), not project_has_role(Pro, S).

:- person(Per), max_hours_working(MAX), #sum{H,Pro: join_project(Per,Pro,H)} > MAX.

% amo
{join_project(Per,Pro, H) : H=1..MAX, max_hours_per_project(MAX)} <= 1 :- person(Per), project(Pro).

% sum
% :- project(Pro), #sum{ H*S, Per: join_project(Per, Pro, H), role(Per,R), salary(R,S) } < B, budget(B).
#amosum{H*S: join_project(Per, Pro, H), role(Per,R), salary(R,S) [(Per, Prog)]} >= B:  budget(B), project(Pro).






