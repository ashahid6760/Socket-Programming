#-*- MakeFile -*-

all: admin.exe 
admin.exe: admin.o cal-new.o 
	gcc admin.o cal-new.o -lpthread -o admin.exe 

admin.o: admin.c
	gcc admin.c -c

cal.o: cal-new.c
	gcc cal-new.c -c

