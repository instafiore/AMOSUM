value_max(20).

{ in_knapsack(X,V) } :- object(X,_,_), value_max(Max), V=0..Max.

% Exactly one multiplicity per object.
&sum { 1 : V=0..Max, in_knapsack(X,V) } = 1 :- object(X,_,_), value_max(Max).

% The native encoding requires total weight to be strictly below MaxWeight.
&sum { V*Weight : object(X,Weight,_), V=0..Max, in_knapsack(X,V) } <= MaxWeight :- ub(MaxWeight,0), value_max(Max).

% Minimum total value.
&sum { V*Value : object(X,_,Value), V=0..Max, in_knapsack(X,V) } >= MinValue :- lb(MinValue,1), value_max(Max).

