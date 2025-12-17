cost(N) :- #sum{ W,X: col(X,C), colour_weight(C,W) } = N.

:- cost(X), expectedCost(Y), Y != X.