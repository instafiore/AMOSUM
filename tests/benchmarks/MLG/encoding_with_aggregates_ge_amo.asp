{a(W,X): weights(W); b(W,X,Y): joint_groups(X,Y), weights(W); b(W,Y,X): joint_groups(Y,X), weights(W)} <= 1 :- groups(X).

:- #sum{W,X : a(W,X); 2*W,X,Y : b(W,X,Y)} < B, lb(B,0).