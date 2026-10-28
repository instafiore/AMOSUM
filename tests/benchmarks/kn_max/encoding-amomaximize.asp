value_max(20).

{in_knapsack(X, V): V=1..MAX, value_max(MAX)} <= 1 :- object(X,WEIGHT,VALUE).


#amosum{ V*WEIGHT, X : in_knapsack(X,V), object(X,WEIGHT,_) [X]} <= MAX_WEIGHT: ub(MAX_WEIGHT,0).
% :- #sum{V*WEIGHT,X: in_knapsack(X,V), object(X,WEIGHT,_)} > MAX_WEIGHT, ub(MAX_WEIGHT,0).

% the value is maximized instead of bounded from below
#amomaximize{ V*VALUE, X : in_knapsack(X,V), object(X,_,VALUE) [X]}.