
% ------------------------------------------------------------
% Sum aggregate
b(1..4,0).
% b(1..4,1).

% lb(weight, aggr_id)
lb(8,0).
% ub(weight, aggr_id)
ub(12,1).

group(a(X),X,0,ID) :- b(X,ID).
% group(d(X),X,0,ID) :- b(X,ID), ID <= 2.
group(d(1),1,0,0).
group(d(2),2,0,0).
group(d(X),X,1,ID) :- b(X,ID).
group(c(X),X,2,ID) :- b(X,ID).

{a(X): b(X,0); d(X): b(X,ID), ID <= 2} <= 1.
{c(X): b(X,0)} <= 1.
{d(X): b(X,0)} <= 1.