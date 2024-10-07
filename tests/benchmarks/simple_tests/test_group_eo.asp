
% ------------------------------------------------------------
% Sum aggregate
b(1..4,0).
b(1..4,1).

% lb(weight, aggr_id)
lb(6,0).
% ub(weight, aggr_id)
ub(7,1).

group(a(X),X,0,ID) :- b(X,ID).
group(c(X),X,1,ID) :- b(X,ID).

{a(X): b(X,0)} = 1.
{c(X): b(X,0)} = 1.




