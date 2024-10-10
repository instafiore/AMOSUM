
:- #sum{V*WEIGHT,X: in_knapsack(X,V), object(X,WEIGHT,_)} > MAX_WEIGHT, ub(MAX_WEIGHT,0).
% group(in_knapsack(X,V), -V*WEIGHT, X,0) :- in_knapsack(X,V), object(X,WEIGHT,_).

% #amosum{ W : in_knapsack(X,V), object(X,_,VALUE), W = V * VALUE @ [X] } >= MIN_VALUE, lb(MIN_VALUE)
% value_max(20).
in_knapsack(X, 1).
in_knapsack(X, 2).
group(in_knapsack(X,V), W, X,1) :- in_knapsack(X,V), object(X,_,VALUE), W = V * VALUE.
{in_knapsack(X, V): in_knapsack(X, V)} <= 1 :- object(X,WEIGHT,VALUE).