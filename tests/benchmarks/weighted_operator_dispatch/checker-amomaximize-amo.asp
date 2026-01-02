cost(N) :- #sum{ E,U,T,S : assign(U,T,S), capable(U,T,E) } = N.
:- cost(X), expectedCost(Y), Y != X.