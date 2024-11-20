{a(W,X): weights(W); b(W,X,Y): joint_groups(X,Y), weights(W); b(W,Y,X): joint_groups(Y,X), weights(W)} <= 1 :- groups(X).

group(a(W,X), W, X,0) :- a(W,X).

% joint groups
group(b(W,X,Y), W, X,0) :- b(W,X,Y).
group(b(W,X,Y), W, Y,0) :- b(W,X,Y).
