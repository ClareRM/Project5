all: coordinator philosopher central_starter token_starter token_philosopher

coordinator: coordinator.c shared.h
	gcc -std=c99 -o coordinator coordinator.c

central_starter: central_starter.c
	gcc -std=c99 -o central_starter central_starter.c

philosopher: philosopher.c philosopher.h shared.h
	gcc -std=c99 -o philosopher philosopher.c

token_starter: token_starter.c
	gcc -std=c99 -o token_starter token_starter.c

token_philosopher: token_philosopher.c token_philosopher.h token_shared.h
	gcc -std=c99 -o token_philosopher token_philosopher.c

clean:
	rm -f coordinator
	rm -f central_starter
	rm -f philosopher