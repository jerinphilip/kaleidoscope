

main: prelude.ll main.cpp
	llc -filetype=obj prelude.ll -o prelude.o 
	clang++ main.cpp prelude.o -o main -static

reverse: prelude.cpp
	clang++ -S -o prelude.cpp.ll prelude.cpp -emit-llvm

clean: 
	rm *.o
	rm main
	rm prelude.cpp.ll

.PHONY: clean
