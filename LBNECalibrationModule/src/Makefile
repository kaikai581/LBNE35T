objects = DeviceInterface.o DeviceManager.o EthernetDevice.o\
          USBDevice.o EmulatedDevice.o RegMap.o EventPacket.o Flash.o
#CXXFLAGS=-fPIC -I../res/include/pandora -I./ -L../res/lib/\
#	 -L/warwick/epp/2012.0/roodaf/5.32.00/x86_64/gnu/4.3.4/lib/root\
#	 -I/warwick/epp/2012.0/roodaf/5.32.00/x86_64/gnu/4.3.4/include/root -L./ \
#	 -I../qscan/fullreco/ -L../qscan/fullreco/ \
#	 -I../qscan/Qscan/ -L../qscan/Qscan/ \
#	 -L/warwick/epp/2012.0/pythia/6.412/x86_64/gnu/4.3.4/lib/

all: libanlBoard.so

#%.exe : %.cxx libQScanInterface.so 
#	$(CXX) $(CXXFLAGS) `root-config --libs` -lPandoraSDK -lLArContent -l LArValidation -lPandoraMonitoring\
#	                   -lQScanInterface  -lboost_program_options -lFullReco -lPythia6 -o $@ $<

libanlBoard.so : $(objects)
	$(CXX) $(CXXFLAGS) --shared $(objects) -o lib/libanlBoard.so

%.o : src/%.cxx src/*.h
	$(CXX) -c $(CXXFLAGS) -o build/$@ $<

clean : 
	rm lib/libanlBoard.so; rm build/*.o
