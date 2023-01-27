all: bfs_seq delta_seq delta_para BF_seq BF_para
bfs_seq: bfs_seq.cpp
	g++ -std=c++11 -o bfs_seq bfs_seq.cpp
delta_seq: delta_seq.cpp
	g++ -std=c++11 -o delta_seq delta_seq.cpp
delta_para: delta_para.cpp
	g++ -std=c++11 -fopenmp -o delta_para delta_para.cpp
BF_seq: BF_seq.cpp
	g++ -std=c++11 -o BF_seq BF_seq.cpp
BF_para: BF_para.cu
	nvcc -std=c++11 -o BF_para BF_para.cu

.PHONY: clean
clean:
	@rm bfs_seq delta_seq delta_para BF_seq BF_para
