all: logger tagemulatenfc tagmanualreadnfc

iso14443a-utils: iso14443a-utils.cpp
	$(CXX) -c iso14443a-utils.cpp -o iso14443a-utils.o

logger: logger.cpp
	$(CXX) -c logger.cpp -o logger.o

tagemulatenfc: tagemulatenfc.cpp logger
	$(CXX) tagemulatenfc.cpp -o tagemulatenfc -lnfc

tagmanualreadnfc: tagmanualreadnfc.cpp logger
	$(CXX) logger.o tagmanualreadnfc.cpp -o tagmanualreadnfc -lnfc
