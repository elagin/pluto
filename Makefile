all:
	g++ -g src/point.cpp src/tools.cpp src/main.cpp  -o bin/pluto -std=c++11 -L/usr/local/lib/ -lboost_system -lsoci_core -lsoci_odbc -ldl -lsoci_postgresql -lconfig++ -lgpx -I/usr/local/include/soci/postgresql/ -I/usr/include/postgresql -I/usr/local/include/soci/


