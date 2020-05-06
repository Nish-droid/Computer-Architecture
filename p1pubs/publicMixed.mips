	lw	1	0	op1	reg[1] <- op1
	lw	2	1	op2	stall
	lw	2	0	op2	no stall
	lw	3	0	op3	reg[3] <- op3
	add	4	1	2	reg[4] <- reg[1] + reg[2]
	lw	4	3	0	forward to r3 but not r4
	addi	4	1	2	no forward
	beqz	0	4	0
	sub	5	4	3	reg[5] <- reg[4] - reg[3]
	sw	4	0	24
	and	4	4	4
	lw	6	0	36
	beqz	0	6	0
done	halt
op1	.fill	8
op2	.fill	3
op3	.fill	8
op4	.fill	8
