all: compileA compileB compileC compileAWS compileClient

compileA:
	gcc -o serverAExec serverA.c

compileB:
	gcc -o serverBExec serverB.c

compileC:
	gcc -o serverCExec serverC.c

compileAWS:
	gcc -o awsExec aws.c

compileClient:
	gcc -o client client.c

serverA:
	./serverAExec

serverB:
	./serverBExec

serverC:
	./serverCExec

aws:
	./awsExec
