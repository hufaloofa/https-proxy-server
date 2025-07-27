EXE=htproxy

$(EXE): main.c lru.c htproxy.c helper.c
	cc -Wall -o $@ $^

format:
	clang-format -style=file -i *.c

clean:
	rm -f $(EXE)
