all: tagemulate

tagemulate: tagemulate.cpp
	$(CXX) tagemulate.cpp -o tagemulate -L "../libnfc/libnfc/" -lnfc
