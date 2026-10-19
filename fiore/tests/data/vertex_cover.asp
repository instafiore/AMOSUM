% minimum-weight vertex cover on a fixed random graph (seed 7)
node(1). node(2). node(3). node(4). node(5). node(6). node(7). node(8). node(9). node(10). node(11). node(12). node(13). node(14). node(15). node(16). node(17). node(18).
w(1,3). w(2,2). w(3,10). w(4,19). w(5,15). w(6,10). w(7,13). w(8,12). w(9,1). w(10,15). w(11,12). w(12,6). w(13,20). w(14,4). w(15,16). w(16,2). w(17,7). w(18,10).
edge(2,3). edge(2,4). edge(2,7). edge(2,8). edge(2,12). edge(2,13). edge(3,4). edge(3,8). edge(3,10). edge(3,14). edge(3,18). edge(4,5). edge(4,7). edge(4,12). edge(4,17). edge(5,11). edge(5,18). edge(6,8). edge(6,10). edge(6,14). edge(7,17). edge(8,10). edge(9,16). edge(10,14). edge(11,12). edge(11,15). edge(11,18). edge(12,15). edge(14,16). edge(14,18). edge(15,16). edge(16,17).
{ in(X) : node(X) }.
:- edge(X,Y), not in(X), not in(Y).
:~ in(X), w(X,W). [W@0, X]
