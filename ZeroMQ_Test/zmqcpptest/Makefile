OUT_DIR = bin
MKDIR_P = mkdir -p

.PHONY: directories

all: directories program

directories: ${OUT_DIR}

${OUT_DIR}:
	${MKDIR_P} ${OUT_DIR}

program:
	g++ -o ${OUT_DIR}/zmqcpptest src/zmqcpptest.cpp -lzmq
