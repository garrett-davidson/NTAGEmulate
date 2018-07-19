all: logger pn532 tagemulatenfc tagmanualreadnfc mfrc522 tagemulatemfrc522 tagmanualreadmfrc522

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

mfrc522: mfrc522.cpp logger
	$(CXX) -c mfrc522.cpp -o mfrc522.o

tagemulatemfrc522: tagemulatemfrc522.cpp mfrc522 logger
	$(CXX) mfrc522.o logger.o tagemulatemfrc522.cpp -o tagemulatemfrc522

tagmanualreadmfrc522: tagmanualreadmfrc522.cpp mfrc522 logger
	$(CXX) mfrc522.o logger.o tagmanualreadmfrc522.cpp -o tagmanualreadmfrc522 -lbcm2835
