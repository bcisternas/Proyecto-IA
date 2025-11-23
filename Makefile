# Makefile para PSP-UAV

all:
	g++ -std=c++17 -O2 main.cpp -o PSP-UAV

clean:
	rm -f PSP-UAV

test:
	./PSP-UAV instancias/PSP-UAV_01_a.txt 5 1000 50
	./PSP-UAV instancias/PSP-UAV_01_b.txt 5 1000 50
	./PSP-UAV instancias/PSP-UAV_02_a.txt 5 1000 50
	./PSP-UAV instancias/PSP-UAV_02_b.txt 5 1000 50
	./PSP-UAV instancias/PSP-UAV_03_a.txt 5 1000 50
	./PSP-UAV instancias/PSP-UAV_03_b.txt 5 1000 50

