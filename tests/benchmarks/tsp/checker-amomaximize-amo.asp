
cost(N) :- #sum{ R,X,Y : tour(X,Y), revenue(X,Y,R) } = N.
:- cost(X), expectedCost(Y), Y != X.