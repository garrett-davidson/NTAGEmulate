all: logger pn532 tagemulatenfc tagmanualreadnfc

iso14443a-utils: iso14443a-utils.cpp
	$(CXX) -c iso14443a-utils.cpp -o iso14443a-utils.o

logger: logger.cpp
	$(CXX) -c logger.cpp -o logger.o

pn532: pn532.cpp
	$(CXX) -c pn532.cpp -o pn532.o

tagemulatenfc: tagemulatenfc.cpp logger pn532
	$(CXX) logger.o pn532.o tagemulatenfc.cpp -o tagemulatenfc -lnfc -lserialport

tagmanualreadnfc: tagmanualreadnfc.cpp logger
	$(CXX) logger.o tagmanualreadnfc.cpp -o tagmanualreadnfc -lnfc
