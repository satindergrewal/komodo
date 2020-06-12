CP=cp
LIB_STD_CPP_6_DLL=/usr/lib/gcc/x86_64-w64-mingw32/9.3-posix/libstdc++-6.dll
LIB_GCC_S_SEH_DLL=/usr/lib/gcc/x86_64-w64-mingw32/9.3-posix/libgcc_s_seh-1.dll
LIB_WIN_P_THREAD_DLL=/usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll
LIB_CURL_DLL=/usr/x86_64-w64-mingw32/bin/libcurl-4.dll

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
	$(CC) -Wall -Wextra -Wfatal-errors -o ../../subatomic.exe subatomic.c subatomic_utils.o -DCURL_STATICLIB -lstdc++ -lstdc++fs -lcurl -lm
	$(CP) -av $(LIB_STD_CPP_6_DLL) $(LIB_GCC_S_SEH_DLL) $(LIB_WIN_P_THREAD_DLL) $(LIB_CURL_DLL) $(WD)/src

clean:
	rm zmigrate oraclefeed subatomic
