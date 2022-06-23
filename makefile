cc = g++

asembler: asembler.o scanner.o 
	$(cc) -o $@ $^

scanner.o: misc/lex.yy.cc
	$(cc) -c -o $@ $^ 

asembler.o: src/asembler.cpp
	$(cc) -c -o $@ $^

emulator: emulator.o
	$(cc) -o $@ $^ -lpthread

emulator.o: src/emulator.cpp
	$(cc) -c -o $@ $^

linker: linker.o
	$(cc) -o $@ $^

linker.o: src/linker.cpp
	$(cc) -c -o $@ $^

sc:
	flex --c++ -o misc/lex.yy.cc misc/flex_proj.l

clean:
	rm misc/lex*
	rm scanner.o
	rm asembler*
	rm linker*
	rm emulator*
	
