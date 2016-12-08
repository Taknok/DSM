ALL: default 

CC           = gcc
CLINKER      = $(CC)
OPTFLAGS     = -O0


SHELL = /bin/sh

CFLAGS  =   -DREENTRANT
CCFLAGS = $(CFLAGS)
LIBS =  -lpthread

EXECS = common.o dsm.o dsmexec dsmwrap truc exemple test

default: $(EXECS)

dsm : dsm.o common.o
	$(CLINKER) $(OPTFLAGS) -o dsm dsm.o common.o $(LIBS)
	mv dsm ./bin

dsmexec: dsmexec.o common.o
	$(CLINKER) $(OPTFLAGS) -o dsmexec dsmexec.o  common.o $(LIBS)
	mv dsmexec ./bin
	
dsmwrap: dsmwrap.o common.o
	$(CLINKER) $(OPTFLAGS) -o dsmwrap dsmwrap.o  common.o $(LIBS)
	mv dsmwrap ./bin
		
truc: truc.o 
	$(CLINKER) $(OPTFLAGS) -o truc truc.o common.o $(LIBS)	
	mv truc ./bin
	

exemple: exemple.o dsm.o 
	$(CLINKER) $(OPTFLAGS) -o exemple exemple.o dsm.o  $(LIBS)
	
test: test.o 
	$(CLINKER) $(OPTFLAGS) -o test test.o common.o  dsm.o $(LIBS)	
	mv test ./bin




clean:
	@-/bin/rm -f *.o *~ PI* $(EXECS) *.out core  
.c:
	$(CC) $(CFLAGS) -o $* $< $(LIBS)
.c.o:
	$(CC) $(CFLAGS) -c $<
.o:
	${CLINKER} $(OPTFLAGS) -o $* $*.o $(LIBS)
