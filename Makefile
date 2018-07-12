all: iso14443a-utils logger tagemulate tagread tagmanualread pn532

iso14443a-utils: iso14443a-utils.cpp
	$(CXX) -c iso14443a-utils.cpp -o iso14443a-utils.o

logger: logger.cpp
	$(CXX) -c logger.cpp -o logger.o

pn532: pn532.cpp logger
	$(CXX) -c pn532.cpp -o pn532.o

tagemulate: tagemulate.cpp pn532 logger
	$(CXX) pn532.o logger.o tagemulate.cpp -o tagemulate -lserialport

tagread: tagread.cpp pn532 logger
	$(CXX) pn532.o logger.o tagread.cpp -o tagread -lserialport

tagmanualread: tagmanualread.cpp pn532 logger
	$(CXX) iso14443a-utils.o pn532.o logger.o tagmanualread.cpp -o tagmanualread -lserialport
