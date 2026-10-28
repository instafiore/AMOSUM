% pick one item from each pair, 2 and 4 are incompatible; optimum: 1, 4, 5 (cost 7)
item(1..6).
w(1,4). w(2,3). w(3,5). w(4,1). w(5,2). w(6,6).
{ in(I) : item(I) }.
:- not in(1), not in(2).
:- not in(3), not in(4).
:- not in(5), not in(6).
:- in(2), in(4).
:~ in(I), w(I,W). [W@0, I]
