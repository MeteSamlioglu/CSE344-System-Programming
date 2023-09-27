target: midterm
midterm: biboServer.c biboClient.c
	gcc biboServer.c -o biboServer -Wall -pthread
	gcc biboClient.c -o biboClient -Wall

.PHONY: makeclean
makeclean:
	rm -f biboServer biboClient
