% three optional atoms, at least one of a, b; optimum: only b (cost 1)
{ a; b; c }.
:- not a, not b.
:~ a. [3@0, a]
:~ b. [1@0, b]
:~ c. [2@0, c]
