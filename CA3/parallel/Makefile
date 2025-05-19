CC = g++
CFLAGS = -std=c++17
LDFLAGS = -lsndfile

all: VoiceFilters.out

VoiceFilters.out: main.cpp
	$(CC) $(CFLAGS) main.cpp -o VoiceFilters.out $(LDFLAGS)

clean:
	rm -f VoiceFilters.out
