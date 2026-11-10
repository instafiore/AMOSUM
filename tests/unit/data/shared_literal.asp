% x is in g1 (tuple a, weight 1) and in g2 (tuple b, weight 2): g1 owns it with weight 3, g2 keeps it as moved
{x;y}. :- x, y. #amosum{1,a: x [g1]; 2,b: x [g2]; 1,c: y [g2]} >= 1.
