
% ------------------------------------------------------------
% Sum aggregate
b(1..4,0).
b(1..4,1).

% lb(weight, aggr_id)
lb(6,0).
% ub(weight, aggr_id)
ub(7,1).

:- #sum{X : a(X); X : c(X)} < B , lb(B,0).
:- #sum{X,a : a(X); X,c : c(X)} > B , ub(B,1).

{a(X): b(X,0)} = 1.
{c(X): b(X,0)} = 1.




