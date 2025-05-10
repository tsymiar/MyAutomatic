#!/bin/bash
sudo apt update
sudo apt install -y build-essential cmake gcc g++ libudev-dev libnl-3-dev libnl-route-3-dev ninja-build pkg-config valgrind python3-dev cython3 python3-docutils pandoc
sudo apt install -y git g++ pkg-config libnl-3-dev libnl-route-3-dev \
  libsystemd-dev flex bison libudev-dev libpci-dev libcap-ng-dev \
  libcmocka-dev libssl-dev libnuma-dev
sudo apt install -y rdma-core
cd ../../3rd
if [ ! -d "rdma-core" ]; then mkdir rdma-core; fi;
cd rdma-core
if [ $(ls ./* | wc -l) -le 1 ]
then
  git submodule update --init --recursive
  git pull
fi
git checkout v57.0 -b v57.0
mkdir build || cd build
cmake -GNinja \
  -DCMAKE_INSTALL_PREFIX=/usr \
  -DCMAKE_BUILD_TYPE=Release \
  -DNO_PYVERBS=1 \
  -DNO_MAN_PAGES=1 \
  ..
ninja && sudo ninja install
cd -
lsmod | grep -E 'rdma_rxe|ib_core'
eth0=$(ip -brief link | awk '{print $1}' | grep en)
sudo rdma link add rxe0 type rxe netdev $eth0
sudo ifconfig $eth0 mtu 9000
sudo ip link set rxe0 up
ibv_devinfo
