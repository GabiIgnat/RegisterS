#fisier folosit pentru compilarea surselor

all:
	gcc RegisterServer/registerS.c -o registerS -l pthread	-Wall
	gcc Client_RC/client.c -o client	-Wall
	gcc Services/HelloServer.c -o SayHello -l pthread	-Wall
	gcc Services/AddNumbersServer.c -o AddNumbers -l pthread	-Wall
clean:
	rm -f registerS client SayHello AddNumbers