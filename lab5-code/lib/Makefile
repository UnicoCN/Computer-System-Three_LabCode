#edit : print.o
#	cc -o edit print.o

#print.o : print.c print.h sbi.c
#	cc -c print.c
#clean : 
#	rm edit print.o


C_SRC       = $(sort $(wildcard *.c))
OBJ                 = $(patsubst %.c,%.o,$(C_SRC))

file = printk.o
all:$(OBJ)
        
%.o:%.c
	${GCC} ${CFLAG} -c $<
clean:
	$(shell rm *.o 2>/dev/null)
