#!/bin/bash
sudo apt-get update
sudo apt-get install -y build-essential cmake gcc g++ libudev-dev libnl-3-dev libnl-route-3-dev ninja-build pkg-config valgrind python3-dev cython3 python3-docutils pandoc
sudo apt-get install -y git g++ pkg-config libnl-3-dev libnl-route-3-dev \
  libsystemd-dev flex bison libudev-dev libpci-dev libcap-ng-dev \
  libcmocka-dev libssl-dev libnuma-dev
sudo apt-get install -y rdma-core
cd ../../3rd
ln -s rxe-dev/include/linux/compiler-gcc5.h rxe-dev/include/linux/compiler-gcc8.h
set container=rockylinux:8
sudo apt-get install -y containerd.io
docker run  -v ../3rd:/mnt:rw -it --rm $container \
    bash -c "yum install -y git gcc gcc-c++ make cmake3 libudev-devel numactl-devel libnl3-devel openssl-devel libcap-ng-devel; if [ ! -f /usr/bin/cmake ]; then ln -s /usr/bin/cmake3 /usr/bin/cmake; fi; ls /mnt; cd /mnt/rxe-dev; make clean; make -j \$(nproc); make install;"
if [ ! -d "rdma-core" ]; then mkdir rdma-core; fi;
cd rdma-core
if [ $(ls . | wc -l) -le 1 ]
then
  git submodule update --init --recursive
fi
if [ "$(git status | grep v57.0)" == "" ]; then
  git config pull.rebase false
  git checkout v57.0
else
  echo "-- pulled v57.0"
fi
if [ -d "build" ]; then rm -rf build/*; else mkdir build; fi;
cd build
cmake -GNinja \
  -DCMAKE_INSTALL_PREFIX=/usr \
  -DCMAKE_BUILD_TYPE=Release \
  -DNO_PYVERBS=1 \
  -DNO_MAN_PAGES=1 \
  ..;
if [ -f "build.ninja" ]; then
  ninja && sudo ninja install
fi
cd -
echo -- Checking RXE
lsmod | grep -E 'rdma_rxe|ib_core'
echo -- Setting RXE
eth0=$(ip -brief link | awk '{print $1}' | grep en)
sudo rdma link add rxe0 type rxe netdev "$eth0"
echo -- Setting RXE MTU
sudo ifconfig "$eth0" mtu 9000 up
sudo ip link set rxe0 up
ibv_devinfo
exit 0
