generatebindings: generatebindings.cpp
	g++ -g -std=c++11 -I/usr/lib/llvm-5.0/include -L/usr/lib/llvm-5.0/lib -o $@ $< -lclang

clean:
	rm generatebindings
