	add	1	0	0	r1 <- result=0
	lw	2	0	mplier	r2 <- multiplier
	lw	3	0	mcand	r3 <- multiplicand
	addi	5	0	1	r5 <- check bit
	addi	6	0	8	r6 <- index (starts at 8)
	addi	7	0	-1	r7 <- -1
	addi	8	0	1	r8 <- constant 1 for shifting
loop	and	4	2	5	see if current bit of mplier==1
	beqz	0	4	skip	if bit is 0, skip the add
	add	1	3	1	add current multiplicand to result
skip	sll	3	3	8	shift mcand left 1 bit by doubling it
	sll	5	5	8	shift check left 1 bit by doubling it
	add	6	7	6	decrement index
	beqz	0	6	end	check if done
	beqz	0	0	loop	jump back to loop
end	sw	1	0	answer
	halt
mcand	.fill	229			multiplicand
mplier	.fill	123			multiplier
answer	.fill	0			answer
