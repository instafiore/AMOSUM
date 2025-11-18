{col(X,C) : colour(C)} <= 1 :- node(X).
:- col(X1,C), col(X2,C), link(X1,X2).
% #amomaximize{ W: col(X,C), colour_weight(C,W) [X]}.
#amosum{ W: col(X,C), colour_weight(C,W) [X]}.


% % metadata
% {col(X, C):colour(C)}<=1:-node(X).
% :-col(X1, C), col(X2, C), link(X1, X2).
% {__group__(col(X, C), "+", W, (X), (0))}:-col(X, C), colour_weight(C, W).
% % __lb__(MIN, (0, lb(MIN, 0))):-lb(MIN, 0).
% __aux__((0), amomaximize).
% {__group__(__aux__((0), ge_amo), "-", MIN, (lb(MIN, 0)), (0, lb(MIN, 0)))}:-lb(MIN, 0).
