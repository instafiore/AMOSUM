% encoding-plain-amo.asp + option-3 Lagrangian-aggregated constraint (implied, hence sound).
% lagr(N,D) is lambda = N/D, produced by lagr.py from the root Dantzig bound.
value_max(20).

{in_knapsack(X, V): V=1..MAX, value_max(MAX)} <= 1 :- object(X,WEIGHT,VALUE).

:- #sum{V*WEIGHT,X: in_knapsack(X,V), object(X,WEIGHT,_)} > MAX_WEIGHT, ub(MAX_WEIGHT,0).

:- #sum{V*VALUE,X: in_knapsack(X,V), object(X,_,VALUE)} < MIN_VALUE, lb(MIN_VALUE,1).

% Order encoding of the multiplicity: geq(X,K) <-> X is taken at least K times.
% (Needed because #sum over in_knapsack(X,V) ignores the AMO and bounds item X by 210*c, not 20*c.)
geq(X,K) :- in_knapsack(X,V), K=1..V.

% sum_X (VALUE - lambda*WEIGHT)*mult(X) >= MIN - lambda*CAP, scaled by D.
coef(X, VALUE*D - WEIGHT*N) :- object(X,WEIGHT,VALUE), lagr(N,D).
rhs(MIN*D - CAP*N)          :- lb(MIN,1), ub(CAP,0), lagr(N,D).
:- #sum{C,X,K: geq(X,K), coef(X,C)} < R, rhs(R).
