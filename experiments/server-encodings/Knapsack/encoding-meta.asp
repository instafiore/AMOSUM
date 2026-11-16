value_max(20).

{ in_knapsack(X,V) } :- object(X,_,_), value_max(MAX), V=1..MAX.

% The weight capacity remains an ordinary ASP constraint. The exactly-one
% constraint for each object and the minimum-value SUM belong to AMOSUM.
:- #sum{V*WEIGHT,X: in_knapsack(X,V), object(X,WEIGHT,_)} > MAX_WEIGHT, ub(MAX_WEIGHT,0).
