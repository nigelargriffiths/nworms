CFLAGS= -g
CFLAGS= -g -O3
LDFLAGS=-lcurses 


nworms:
	cc $(CFLAGS) -o nworms nworms.c $(LDFLAGS) 

wormp:
	cc -o wormp wormp.c -lpthread $(LDFLAGS) 
