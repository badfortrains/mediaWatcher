libMicro.a: MediaFinder.o
	ar -cvq libMicro.a MediaFinder.o

mmc: main.o MediaFinder.o
	g++ -O2 -g -o  mmc main.o MediaFinder.o -L. -lPlatinum -lPltMediaRenderer -lZlib -lNeptune -lPltMediaConnect -lPltMediaServer -lpthread -laxTLS -fno-stack-protector

main.o: main.cpp
	g++ -O2 -ggdb -g -c main.cpp -I./node_modules/Platinum/Platinum/Source/Platinum -I./node_modules/Platinum/Platinum/Source/Core -I./node_modules/Platinum/Platinum/Source/../../Neptune/Source/Core -I./node_modules/Platinum/Platinum/Source/Platinum -I./node_modules/Platinum/Platinum/Source/Devices/MediaServer -I./node_modules/Platinum/Platinum/Source/Platinum -I./node_modules/Platinum/Platinum/Source/Devices/MediaServer -I./node_modules/Platinum/Platinum/Source/Devices/MediaRenderer -I./node_modules/Platinum/Platinum/Source/Devices/MediaConnect -I./node_modules/Platinum/Platinum/Source/Extras -fno-stack-protector

MediaFinder.o: MediaFinder.cpp
	g++ -O2 -c -g -ggdb MediaFinder.cpp -I./node_modules/Platinum/Platinum/Source/Platinum -I./node_modules/Platinum/Platinum/Source/Core -I./node_modules/Platinum/Platinum/Source/../../Neptune/Source/Core -I./node_modules/Platinum/Platinum/Source/Platinum -I./node_modules/Platinum/Platinum/Source/Devices/MediaServer -I./node_modules/Platinum/Platinum/Source/Platinum -I./node_modules/Platinum/Platinum/Source/Devices/MediaServer -I./node_modules/Platinum/Platinum/Source/Devices/MediaRenderer -I./node_modules/Platinum/Platinum/Source/Devices/MediaConnect -I./node_modules/Platinum/Platinum/Source/Extras -fno-stack-protector

clean:
	rm -rf *o mmc libMicro.a ./build/Release/*
