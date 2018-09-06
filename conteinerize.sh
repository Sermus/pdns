#! /bin/bash
docker build -f Dockerfile.pdns_build . -t sermus/pdns_build
docker run --rm sermus/pdns_build
docker build . -t sermus/pdns
