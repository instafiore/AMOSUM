object(1,100,1000).
object(2,200,4000).
object(3,120,2500).
object(4,100,1000).
object(5,200,4000).
% object(6,120,2500).
% object(7,100,1000).
% object(8,200,4000).
% object(9,120,2500).


value_max(20).

{in_knapsack(X, V): V=0..MAX, value_max(MAX)} = 1 :- object(X,WEIGHT,VALUE).

% aggregates
:- #sum{-V*WEIGHT,X: in_knapsack(X,V), object(X,WEIGHT,_)} < MAX_WEIGHT, lb(MAX_WEIGHT,0).

:- #sum{V*VALUE,X: in_knapsack(X,V), object(X,_,VALUE)} < MIN_VALUE, lb(MIN_VALUE,1).
