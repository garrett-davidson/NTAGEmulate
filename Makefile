all: tagemulate tagread pn532

pn532: pn532.cpp
	$(CXX) -c pn532.cpp -o pn532.o

tagemulate: tagemulate.cpp pn532
	$(CXX) pn532.o tagemulate.cpp -o tagemulate -lserialport

tagread: tagread.cpp pn532
	$(CXX) pn532.o tagread.cpp -o tagread -lserialport
