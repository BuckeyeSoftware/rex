# Velocity
li %rv0, ${0.0, 0.0, 0.0, 0.0}
rnd %rs1
li %rs16, $5.0
mul %rs1, %rs1, %rs16
mov %cv0, %rv0

# Position
mov %cv2, %pv0  # rv0 = pv0

# Color
rnd %rs0
rnd %rs1
rnd %rs2
li %rs3, $1.0
mov %cv3, %rv0

# Life
rnd %rs0
li %rs1, $5.0
mul %rs0, %rs0, %rs1
mov %cs4, %rs0

# Size
rnd %rs0
li %rs1, $10.0
mul %rs0, %rs0, %rs1
mov %cs5, %rs0