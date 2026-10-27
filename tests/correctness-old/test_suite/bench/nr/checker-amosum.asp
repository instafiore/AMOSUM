:- nurse(N), maxHoursPerYear(MAX), #sum{H,D : assign(N,T,D), workshift(T,_,H)} > MAX.
:- nurse(N), minHoursPerYear(MIN), #sum{H,D : assign(N,T,D), workshift(T,_,H), H > 0} < MIN.