
% ------------------------------------------------------------
% Sum aggregate

% lb(weight, aggr_id)
lb(6,0).

group(a,1,0,0).
group(d,1,0,0).
group(b,2,0,0).

group(b,2,1,0).
group(c,5,1,0).

{a;b;d} <= 1.
{b;c} <= 1.