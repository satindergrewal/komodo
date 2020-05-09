# just type make to compile all dapps
all: zmigrate oraclefeed subatomic

zmigrate:
	$(CC) zmigrate.c -o zmigrate -lm

oraclefeed:
	$(CC) oraclefeed.c -o oraclefeed -lm

subatomic:
	$(CXX) -c -std=c++17 -Wfatal-errors -Wall -Wextra -Werror subatomic_utils.cpp
	$(CC) -Wall -Wextra -Wfatal-errors -o ../../subatomic subatomic.c subatomic_utils.o -lstdc++ -lstdc++fs -lcurl -lm

clean:
	rm zmigrate oraclefeed subatomic
