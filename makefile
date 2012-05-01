libMicro.a: MediaFinder.o
	ar -cvq libMicro.a MediaFinder.o

mmc: main.o MediaFinder.o
	g++ -O2 -g -o  mmc main.o MediaFinder.o -L. -lPlatinum -lPltMediaRenderer -lZlib -lNeptune -lPltMediaConnect -lPltMediaServer -lpthread -laxTLS -fno-stack-protector

main.o: main.cpp
	g++ -O2 -ggdb -g -c main.cpp -I../../Platinum -I../../Core -I../../../../Neptune/Source/Core -I../../Platinum -I../../Devices/MediaServer -I../../Platinum -I../../Devices/MediaServer -I../../Devices/MediaRenderer -I../../Devices/MediaConnect -I../../Extras -fno-stack-protector

MediaFinder.o: MediaFinder.cpp
	g++ -O2 -c -g -ggdb MediaFinder.cpp -I../../Platinum -I../../Core -I../../../../Neptune/Source/Core -I../../Platinum -I../../Devices/MediaServer -I../../Platinum -I../../Devices/MediaServer -I../../Devices/MediaRenderer -I../../Devices/MediaConnect -I../../Extras -fno-stack-protector

clean:
	rm -rf *o mmc libMicro.a ./build/Release/*
