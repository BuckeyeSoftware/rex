# Velocity
li %rv0, ${0.0, 3.0, 0.0, 0.0}
mov %cv0, %rv0

# Position
li %rs4, $10.0
rnd %rs0
mul %rs0, %rs4, %rs0 # x
li %rs1, $0.0        # y
rnd %rs2             # z
mul %rs2, %rs4, %rs2 # z
# li %rv0, ${0.0, 0.0, 0.0, 0.0}
mov %cv2, %rv0

# Color
rnd %rs0
rnd %rs1
rnd %rs2
li %rs3, $1.0
mov %cv3, %rv0

# Life
rnd %rs0
li %rs1, $10.0
mul %rs0, %rs0, %rs1
mov %cs4, %rs0

# Size
rnd %rs0
li %rs1, $10.0
mul %rs0, %rs0, %rs1
mov %cs5, %rs0