hotswap: hotswap.cpp hotswap.h hotswap.pb
	g++ -I/usr/include -L/usr/lib/ hotswap.cpp hotswap.pb.cc -o ../bin/hotswap.out -lprotobuf

hotswap.pb: hotswap.proto
	protoc --cpp_out=. hotswap.proto

clean:
	rm -f hotswap.pb.h hotswap.pb.cc ../bin/hotswap.out

