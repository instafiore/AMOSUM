import re,sys
from fractions import Fraction
for f in sys.argv[1:]:
    t=open(f).read(); o=[tuple(map(int,m)) for m in re.findall(r'object\((\d+),(\d+),(\d+)\)',t)]
    cap=int(re.search(r'ub\((\d+),0\)',t).group(1)); mn=int(re.search(r'lb\((\d+),1\)',t).group(1))
    def dantz(mult):
        r=cap; ub=Fraction(0)
        for x,w,v in sorted(o,key=lambda a:Fraction(a[2],a[1]),reverse=True):
            c,val=mult*w,mult*v
            if c<=r: r-=c; ub+=val
            else: ub+=Fraction(val*r,c); break
        return float(ub)
    print(f.split('/')[-1], "MIN",mn,"LP(0..20)",round(dantz(20)),"flat over in_knapsack(X,1..20) (=0..210 copies)",round(dantz(210)))
