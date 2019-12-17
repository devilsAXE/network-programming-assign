all: publisher broker subscriber
        
publisher : Publisher.c
	gcc pubsub.h -o pub Publisher.c

broker : Broker.c
	gcc pubsub.h -o bro Broker.c -lpthread

subscriber : Subscriber.c
	gcc pubsub.h -o pub Subscriber.c 
