:- col(X1,C), col(X2,C), link(X1,X2).

% #amosum{ W : col(X,C), colour_weight(C,W) [X] }
group(col(X,C), W, X,0) :- col(X,C), colour_weight(C,W).
{col(X,C) : node(X), colour(C) } <= 1 .