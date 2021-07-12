
# This is a simple sphere cloud emitter
#
# %ps0 = radius
# %cv2 = position
#
# theta = 2.0 * M_PI * rnd01()
# phi   = acos(1.0 - 2.0 * rnd01())
# r     = sqrt(rnd01() * radius)
# x     = r * sin(phi) * cos(theta)
# y     = r * sin(phi) * sin(theta)
# z     = r * cos(phi)

# :: velocity (25 units on y axis)
li %rv0, ${0.0, 25.0, 0.0, 0.0}
mov %cv0, %rv0

# :: color (random RGBA)
rnd %rv0
mov %cv3, %rv0

# :: life (random 0 to 2 seconds)
rnd %rs0             # rs0 = rnd01()
li %rs1, $2.0        # rs1 = 4.0
mul %rs0, %rs0, %rs1 # rs0 = rs0 * rs1
mov %cs0, %rs0       # life = rs0

# :: size (random 0 to 25 units)
# rnd %rs0             # rs0 = rnd01()
li %rs0, $50.0       # rs1 = 10.0
# mul %rs0, %rs0, %rs1 # rs0 = rs0 * rs1
mov %cs1, %rs0       # size = rs0

# :: position
# li %rv0 ${0.0, 0.0, 0.0, 0.0}
# mov %cv2, %rv0

# load some common scalars as a vector load immediate
# rs0 = 1.0
# rs1 = 2.0
# rs2 = 6.28 (2.0 * M_PI)
# rs3 = rnd(0, 1)
li %rv0, ${1.0, 2.0, 6.28, 0.0}
rnd %rs3

# :: theta (rs3)
mul %rs3, %rs2, %rs3

# :: phi (rs4)
rnd %rs4
mul %rs4, %rs1, %rs4
sub %rs4, %rs0, %rs4
acos %rs4, %rs4

# :: r (rs5)
rnd %rs5
mov %rs6, %ps0
mul %rs5, %rs5, %rs6
sqrt %rs5, %rs5

# :: x (rs8) = rs5 * sin(rs4) * cos(rs3)
cos %rs6, %rs3
sin %rs7, %rs4
mul %rs6, %rs6, %rs7
mul %rs6, %rs5, %rs6
mov %rs8, %rs6

# :: y = rs5 * sin(rs4) * sin(rs3)
sin %rs6, %rs3
sin %rs7, %rs4
mul %rs6, %rs6, %rs7
mul %rs6, %rs5, %rs6
mov %rs9, %rs6

# :: z = rs5 * cos(rs4)
cos %rs6, %rs4
mul %rs6, %rs5, %rs6
mov %rs10, %rs6

# :: position
mov %cv2, %rv2