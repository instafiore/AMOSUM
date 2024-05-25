group(x,1,1,0).
group(y,2,1,0).
group(z,2,2,0).
group(w,3,2,0).
lb(3,0).
{x;y;z;w}.
:- #count{x:x;y:y} > 1.
:- #count{z:z;w:w} > 1.


% :- #sum{1,x:x; 2,y:y; 2,z:z; 3,w:w} < 3.