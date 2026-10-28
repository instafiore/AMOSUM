% x(5) is false before search: at the maximum possible value the bound is violated at init (the conflict
% must hold only for that solve call, or every lower bound would be UNSAT too)
{x(1..5)}. c :- x(5). :- c. #amomaximize{I: x(I) [g(I)]}.
