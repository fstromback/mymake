all:
	c++ *.cpp -o mymake
	./mymake -s ./ -o build/ -e my-make -c c++
