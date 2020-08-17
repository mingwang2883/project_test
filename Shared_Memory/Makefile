.PHONY:all
all:server client

client:client.c comm.c
	gcc -o $@ $^ -lpthread
	#arm-anykav200-linux-uclibcgnueabi-gcc -o $@ $^
server:server.c comm.c
	gcc -o $@ $^ -lpthread
	#arm-anykav200-linux-uclibcgnueabi-gcc -o $@ $^

.PHONY:clean
clean:
	rm -f client server
