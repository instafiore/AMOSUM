object(1,100,1000).
object(2,200,4000).
object(3,120,2500).
max_weight(1000).
min_value(1200).
value_max(20).

{in_knapsack(X, V): V=0..MAX, value_max(MAX)} = 1 :- object(X,WEIGHT,VALUE).

:- #sum{V*WEIGHT,X: in_knapsack(X,V), object(X,WEIGHT,_)} >= MAX_WEIGHT, max_weight(MAX_WEIGHT).
:- #sum{V*VALUE,X: in_knapsack(X,V), object(X,_,VALUE)} < MIN_VALUE, min_value(MIN_VALUE).