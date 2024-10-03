
% ------------------------------------------------------------
% Sum aggregate
b(1..4,0).

% lb(weight, aggr_id)
ub(7,0).
lb(3,0).

group(a(X),X,0,ID) :- b(X,ID).
group(c(X),X,1,ID) :- b(X,ID).

{a(X): b(X,0)} <= 1.
{c(X): b(X,0)} <= 1.
