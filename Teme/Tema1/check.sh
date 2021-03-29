#!/bin/bash

cd src && sudo make && cd -
sudo ln -fs src/router .

sudo fuser -k 6653/tcp
sudo python3 ./topo.py tests
