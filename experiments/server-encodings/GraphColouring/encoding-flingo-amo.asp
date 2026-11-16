{ col(X,C) } :- node(X), colour(C).

% At most one colour per node (the AMO constraint used by the benchmark).
&sum { 1 : colour(C), col(X,C) } <= 1 :- node(X).

% Adjacent nodes cannot use the same colour.
:- col(X1,C), col(X2,C), link(X1,X2).

% Weighted global lower bound.
&sum { W : node(X), colour_weight(C,W), col(X,C) } >= B :- lb(B,0).

