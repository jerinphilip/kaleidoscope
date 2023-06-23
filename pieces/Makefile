

main: prelude.ll main.cpp
	llc -filetype=obj prelude.ll -o prelude.o 
	clang++ -O3 main.cpp prelude.o -o main -static

reverse: prelude.cpp
	clang++ -S -o prelude.cpp.ll prelude.cpp -emit-llvm

tiny: tiny.elf64.asm
	nasm -f bin -o tiny.elf64 tiny.elf64.asm && chmod +x tiny.elf64
	./tiny.elf64 || echo "$$?" # Escaping $ for `make`.

tinyplus: tiny+max.elf64.asm
	nasm -f bin -o tiny+max.elf64 tiny+max.elf64.asm && chmod +x tiny+max.elf64
	./tiny+max.elf64 || echo "$$?" # Escaping $ for `make`.

tinyplus-O0: tiny+max-O0.elf64.asm
	nasm -f bin -o tiny+max-O0.elf64 tiny+max-O0.elf64.asm && chmod +x tiny+max-O0.elf64
	./tiny+max.elf64 || echo "$$?" # Escaping $ for `make`.

clean: 
	rm *.o
	rm main
	rm prelude.cpp.ll

.PHONY: clean
