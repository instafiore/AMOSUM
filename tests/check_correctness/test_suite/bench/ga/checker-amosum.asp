:- project(Pro, LB, UB), #sum{ H*S, Per, M: join_project(Per, Pro, H, M), role(Per,R), salary(R,S), project_month(Pro, SM, EM), M >= SM, M <= EM} < LB.
% sum(Pro, X) :- project(Pro, LB, UB), X = #sum{ H*S, Per, M: join_project(Per, Pro, H, M), role(Per,R), salary(R,S), project_month(Pro, SM, EM), M >= SM, M <= EM}.
