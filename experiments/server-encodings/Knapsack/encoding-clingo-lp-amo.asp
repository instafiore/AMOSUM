value_max(20).

{ in_knapsack(X,V) } :- object(X,_,_), value_max(Max), V=0..Max.

% Boolean-to-numeric channeling.
&dom { 0..1 } = lp_in_knapsack(X,V) :- object(X,_,_), value_max(Max), V=0..Max.
&sum { lp_in_knapsack(X,V) } = 1 :- object(X,_,_), value_max(Max), V=0..Max, in_knapsack(X,V).
&sum { lp_in_knapsack(X,V) } = 0 :- object(X,_,_), value_max(Max), V=0..Max, not in_knapsack(X,V).

% Exactly one multiplicity per object.
&sum { lp_in_knapsack(X,V) : V=0..Max } = 1 :- object(X,_,_), value_max(Max).

% The native encoding requires total weight to be strictly below MaxWeight.
&sum { (V*Weight)*lp_in_knapsack(X,V) : object(X,Weight,_), V=0..Max } <= MaxWeight :- ub(MaxWeight,0), value_max(Max).

% Minimum total value.
&sum { (V*Value)*lp_in_knapsack(X,V) : object(X,_,Value), V=0..Max } >= MinValue :- lb(MinValue,1), value_max(Max).

