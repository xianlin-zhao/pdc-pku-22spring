all: histeq sharpen
histeq: histeq.cpp
	g++ -w -lOpenCL -o histeq histeq.cpp
sharpen: sharpen.cpp
	g++ -w -lOpenCL -o sharpen sharpen.cpp

.PHONY: clean
clean:
	@rm histeq sharpen