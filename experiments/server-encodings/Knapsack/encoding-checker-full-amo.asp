%%%%% full checker of encoding-plain-amo.asp %%%%%
% Input: the instance facts and the answer set as facts (all atoms).
% The choice rule of the plain encoding becomes a constraint, so the answer
% set is taken as complete: atoms missing from the output are false.
% in_knapsack(X,0) (object not taken, printed by flingo and clingo[LP]) is
% ignored: it is not an atom of the plain encoding and adds no weight or value.

#defined in_knapsack/2.

value_max(20).
multiplicity(X,V) :- object(X,_,_), value_max(MAX), V=1..MAX.

% Only objects and multiplicities of the instance.
:- in_knapsack(X,V), V != 0, not multiplicity(X,V).

% At most one multiplicity per object (choice rule of the plain encoding).
:- object(X,_,_), #count{ V : in_knapsack(X,V), multiplicity(X,V) } > 1.

% Total weight at most MAX_WEIGHT, total value at least MIN_VALUE.
:- #sum{V*WEIGHT,X: in_knapsack(X,V), object(X,WEIGHT,_)} > MAX_WEIGHT, ub(MAX_WEIGHT,0).
:- #sum{V*VALUE,X: in_knapsack(X,V), object(X,_,VALUE)} < MIN_VALUE, lb(MIN_VALUE,1).
