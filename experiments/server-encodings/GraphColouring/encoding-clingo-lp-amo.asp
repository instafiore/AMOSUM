{ col(X,C) } :- node(X), colour(C).

% Boolean-to-numeric channeling.
&dom { 0..1 } = lp_col(X,C) :- node(X), colour(C).
&sum { lp_col(X,C) } = 1 :- node(X), colour(C), col(X,C).
&sum { lp_col(X,C) } = 0 :- node(X), colour(C), not col(X,C).

% At most one colour per node (the AMO constraint used by the benchmark).
&sum { lp_col(X,C) : colour(C) } <= 1 :- node(X).

% Adjacent nodes cannot use the same colour.
:- col(X1,C), col(X2,C), link(X1,X2).

% Weighted global lower bound.
&sum { W*lp_col(X,C) : node(X), colour_weight(C,W) } >= B :- lb(B,0).


