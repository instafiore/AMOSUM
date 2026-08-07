{ x(I) } :- bid(I, _).
shares_item(Bid1, Bid2) :- package(Bid1, Item), package(Bid2, Item), Bid1 < Bid2.
:- x(Bid1), x(Bid2), shares_item(Bid1, Bid2).
:- #sum{ Value, I : x(I), bid(I, Value) } < B, lb(B).