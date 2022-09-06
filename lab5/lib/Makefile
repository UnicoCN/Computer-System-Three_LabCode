VPATH = ../include:../arch/riscv/include
all : printk.o rand.o string.o
	${GCC} ${CFLAG} -o all printk.o rand.o string.o
printk.o : printk.c
	${GCC} ${CFLAG} -c printk.c
rand.o : rand.c
	${GCC} ${CFLAG} -c rand.c
string.o : string.c
	${GCC} ${CFLAG} -c string.c
.PHONY : clean
clean:
	rm all printk.o rand.o string.o
