
SUBATOMIC_LDFLAGS="-L../../../depends/x86_64-w64-mingw32/lib/"
SUBATOMIC_CURL_STATIC="-L/../../../depends/x86_64-w64-mingw32/lib/libcurl.a"
SUBATOMIC_CFLAGS="-I../../../depends/x86_64-w64-mingw32/include/"

# just type make to compile all dapps
all: zmigrate oraclefeed subatomic

zmigrate:
	$(CC) zmigrate.c -o zmigrate -lm

oraclefeed:
	$(CC) oraclefeed.c -o oraclefeed -lm

subatomic:
	$(CXX) -c -std=c++17 -Wfatal-errors -Wall -Wextra -Werror subatomic_utils.cpp
	$(CC) -Wall -Wextra -Wfatal-errors -o ../../subatomic subatomic.c subatomic_utils.o -lstdc++ -lstdc++fs -lcurl -lm

subatomic_win:
	$(CXX) -c -std=c++17 -Wfatal-errors -Wall -Wextra -Werror subatomic_utils.cpp
	$(CC) -Wall -Wextra -Wfatal-errors -o ../../subatomic.exe subatomic.c subatomic_utils.o $(SUBATOMIC_CFLAGS) $(SUBATOMIC_LDFLAGS) -lcurl -lstdc++ -lstdc++fs -lm  

clean:
	rm zmigrate oraclefeed subatomic
