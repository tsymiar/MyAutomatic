#!/bin/bash
# ── helper: check if a dpkg package is properly installed ──
pkg_ok() {
    dpkg -s "$1" 2>/dev/null | grep -q '^Status: install ok installed'
}
# ── collect missing packages ──
MISSING_PKGS=()
needs() { for pkg in "$@"; do pkg_ok "$pkg" || MISSING_PKGS+=("$pkg"); done; }
needs build-essential cmake gcc g++ libudev-dev libnl-3-dev libnl-route-3-dev \
    ninja-build pkg-config valgrind python3-dev cython3 python3-docutils pandoc
needs git libsystemd-dev flex bison libpci-dev libcap-ng-dev \
    libcmocka-dev libssl-dev libnuma-dev
needs rdma-core
if [ ${#MISSING_PKGS[@]} -gt 0 ]; then
    echo "==> Installing missing: ${MISSING_PKGS[*]}"
    sudo apt-get update
    sudo apt-get install -y "${MISSING_PKGS[@]}"
else
    echo "==> All apt packages are already installed, skipping."
fi
cd ../../3rd
if [ "${MAKE_RXE}" == "1" ]; then
  needs containerd.io
  docker run  -v "../3rd:/mnt:rw" -it --rm "rockylinux:8" \
   bash -c  \
    "yum install -y git gcc gcc-c++ make cmake3 findutils bc libudev-devel numactl-devel libnl3-devel openssl-devel libcap-ng-devel; if [ ! -f /usr/bin/cmake ]; then ln -s /usr/bin/cmake3 /usr/bin/cmake; fi; cd /mnt/rxe-dev; ls -al; make clean; cat include/linux/compiler-gcc5.h > include/linux/compiler-gcc8.h; make -j \$(nproc); make install;"
fi
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
