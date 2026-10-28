% maximize x+y with x+y <= 4 (x, y in 1..3): optimum 4
{x(1..3)} = 1. {y(1..3)} = 1.
#amosum{I,x: x(I) [gx]; I,y: y(I) [gy]} <= 4.
#amomaximize{I,x: x(I) [gx]; I,y: y(I) [gy]}.
