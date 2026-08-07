{choice(X,Y) : groupsize(Z), Y=1..Z} = 1 :- groupnumber(N), X=1..N.
:- #sum{W,X,Y: choice(X,Y), W=Y} < B, lb(B, 1).
