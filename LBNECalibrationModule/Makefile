objects = build/DeviceInterface.o build/DeviceManager.o build/EthernetDevice.o\
          build/USBDevice.o build/EmulatedDevice.o build/RegMap.o build/EventPacket.o\
          build/Log.o build/Flash.o
CXXFLAGS=-fPIC -Isrc/ -Llib/ -std=c++11 -Iinclude\
	 -I/data/lbnedaq/products/boost/v1_56_0/source/boost_1_56_0/ -Iinclude/tclap-1.2.1/include\
	 -I/data/lbnedaq/scratch/sklin/local/include\
	 -L/data/lbnedaq/products/boost/v1_56_0/Linux64bit+2.6-2.12-e6-prof/lib/\
	 -L/data/lbnedaq/scratch/sklin/Software/ZeroMQ/lib\
	 -L/data/lbnedaq/scratch/sklin/local/lib
all: libanlBoard.so lcmtest.exe vmon.exe

%.exe : app/%.cxx lib/libanlBoard.so
	$(CXX) $(CXXFLAGS) -lanlBoard -lboost_system -lftd2xx -lzmq -lconfig++ src/jsoncpp.cpp -o bin/$@ $<

libanlBoard.so : $(objects)
	$(CXX) $(CXXFLAGS) --shared $(objects) -o lib/libanlBoard.so

build/%.o : src/%.cxx src/*.h
	$(CXX) -c $(CXXFLAGS) -o $@ $<

clean : 
	rm lib/libanlBoard.so; rm build/*.o
