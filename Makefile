CC       = env OMPI_CC=clang openmpicc
CXX      = env OMPI_CXX=clang++ openmpic++
CPPFLAGS = -I/opt/local/include -DBOOST_MPI_HOMOGENEOUS
CCFLAGS  = -Wall -Wno-deprecated-declarations -g -std=c99   -march=native
CXXFLAGS = -Wall -Wno-deprecated-declarations -g -std=c++11 -march=native
LDFLAGS  = -L/opt/local/lib
LIBS     = -lboost_mpi-mt -lboost_serialization-mt

SRCS = rpc_main.cc rpc_server.cc demo.cc
OBJS = ${SRCS:.cc=.o}
EXE  = demo



all: ${EXE}

${EXE}: ${OBJS}
	${CXX} ${CPPFLAGS} ${CXXFLAGS} ${LDFLAGS} -o $@ $^ ${LIBS}

%.o: %.c
	${CC} ${CPPFLAGS} ${CFLAGS} -c $*.c

%.o: %.cc
	${CXX} ${CPPFLAGS} ${CXXFLAGS} -c $*.cc

clean:
	${RM} ${OBJS} ${EXE}

depend:
	makedepend ${SRCS}

.PHONY: all clean depend



# DO NOT DELETE

rpc_main.o: rpc_main.hh rpc_server.hh rpc_server_mpi.hh rpc_call.hh
rpc_main.o: rpc_global_ptr.hh rpc_tuple.hh
rpc_server.o: rpc_server.hh
demo.o: rpc.hh rpc_call.hh rpc_global_ptr.hh rpc_server.hh rpc_tuple.hh
demo.o: rpc_main.hh
