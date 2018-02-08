#!/bin/bash
python3 GenDockerfile.py ../../.travis.yml > Dockerfile.easy &&
  docker build -t easy/test -f Dockerfile.easy . 
