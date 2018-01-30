#!/bin/bash
docker build -t easy/llvm -f Dockerfile.llvm . && \
	docker build -t easy/test -f Dockerfile.easy . && \
	docker run -t easy/test /usr/local/bin/lit /easy-jit/_build/tests --verbose
