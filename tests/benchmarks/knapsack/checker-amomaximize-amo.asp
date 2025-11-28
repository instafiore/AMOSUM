cost(N) :- #sum{ V*VALUE,X : in_knapsack(X,V), object(X,_,VALUE)} = N.

% :- cost(X), expectedCost(Y), Y != X.