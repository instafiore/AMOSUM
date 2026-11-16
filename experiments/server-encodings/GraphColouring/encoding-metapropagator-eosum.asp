propagator graph_colouring_assignment:

% Esattamente un colore C per ogni nodo X
__count__(X, col(X,C)) :- node(X), colour(C).

__count__(X, "=", 1) :- node(X).


% Peso totale dei colori assegnati:
% sum{W : col(X,C), colour_weight(C,W)} >= B
__sum__(B, col(X,C), W) :- lb(B,0), node(X), colour_weight(C,W).

__sum__(B, ">=", B) :- lb(B,0).
