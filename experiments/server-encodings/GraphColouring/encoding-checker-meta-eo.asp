:- node(X), #count{ C : col(X,C), colour(C) } != 1.
:- #sum{W,X: col(X,C), colour_weight(C,W)} < B, lb(B,0).