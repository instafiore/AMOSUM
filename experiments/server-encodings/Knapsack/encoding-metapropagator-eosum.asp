propagator knapsack_assignment:

% Esattamente un valore V per ogni oggetto X
__count__(X, in_knapsack(X,V)) :- object(X,_,_), value_max(MAX), V = 1..MAX.
__count__(X, "=", 1) :- object(X,_,_).


% Valore totale dello zaino:
% sum{V*VALUE : in_knapsack(X,V)} >= B
__sum__(B, in_knapsack(X,V), V*VALUE) :- lb(B,1), object(X,_,VALUE), value_max(MAX), V = 1..MAX.

__sum__(B, ">=", B) :- lb(B,1).
