% choose  at most one outgoing edge from each city
{ tour(X,Y) : revenue(X,Y,_) } <= 1 :- city(X).

% choose at most one incoming edge to each city
:- city(Y), tour(X1,Y), tour(X2,Y), X1 != X2.

% reachability
reachable(Y) :- start(X), tour(X,Y).
reachable(Y) :- reachable(X), tour(X,Y).

% every city must be reachable
:- city(X), not reachable(X).

#maximize{ R,X,Y : tour(X,Y), revenue(X,Y,R) }.

