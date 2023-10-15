FROM ubuntu
RUN apt-get update && apt-get install -y gcc \
    make \
    cmake \
    python3 \
    python3-pip \
    git
RUN pip install msgpack-rpc-python

WORKDIR /midterm_project
RUN git clone https://github.com/rpclib/rpclib.git && \ 
    cd rpclib && \
    mkdir build && \
    cd ./build && \
    cmake .. && \
    cmake --build . && \
    make install   
    # Compile rpclib
RUN apt-get install -y net-tools 
COPY ./chord/  ./chord
COPY ./test_scripts ./test_scripts
WORKDIR /midterm_project/chord
RUN make
    # Compile chord.cc, chord.h, rpcs.h