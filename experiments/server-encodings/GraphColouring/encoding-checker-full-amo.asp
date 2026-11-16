%%%%% full checker of encoding-plain-amo.asp %%%%%
% Input: the instance facts and the answer set as facts (all atoms).
% The choice rule of the plain encoding becomes a constraint, so the answer
% set is taken as complete: atoms missing from the output are false.

#defined col/2.

% Only nodes and colours of the instance.
:- col(X,C), not node(X).
:- col(X,C), not colour(C).

% At most one colour per node (choice rule of the plain encoding).
:- node(X), #count{ C : col(X,C), colour(C) } > 1.

% Adjacent nodes cannot use the same colour.
:- col(X1,C), col(X2,C), link(X1,X2).

% Weighted global lower bound.
:- #sum{W,X: col(X,C), colour_weight(C,W)} < B, lb(B,0).
