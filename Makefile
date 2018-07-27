all: iso14443a-utils logger tagemulate tagread tagmanualread pn532 poll pn532speed listenpassive

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

poll: poll.cpp
	$(CXX) poll.cpp -o poll -lnfc

pn532speed: pn532speed.cpp
	$(CXX) -c pn532speed.cpp -o pn532speed.o

listenpassive: listenpassive.cpp pn532speed
	$(CXX) pn532speed.o listenpassive.cpp -o listenpassive
