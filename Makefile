INCLUDEDIR=`llvm-config --includedir`
LIBDIR=`llvm-config --ldflags`

generatebindings: generatebindings.cpp
	g++ -std=c++11 -I$(INCLUDEDIR) $(LIBDIR) -o $@ $< -lclang

clean:
	rm generatebindings
