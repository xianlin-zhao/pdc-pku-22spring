all: psrs conv
psrs: psrs.cpp
	mpic++ -o psrs psrs.cpp
conv: conv.cpp
	mpic++ -o conv conv.cpp

.PHONY : clean
clean:
	@rm psrs conv