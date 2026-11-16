{ col(X,C) } :- node(X), colour(C).

% Adjacent nodes still cannot use the same colour. The exactly-one constraint
% for each node and the weighted lower-bound SUM belong to AMOSUM.
:- col(X1,C), col(X2,C), link(X1,X2).
