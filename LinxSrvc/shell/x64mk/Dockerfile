# amd64 Android build-box for building Device2Device on aarch64 hosts (Jetson).
# Runs entirely under QEMU user-mode emulation (see plan B).
#
# NOTE: gradle, Android SDK and NDK are expected to be MOUNTED from the host,
# they are intentionally NOT baked into the image:
#   - gradle    : pure java, arch-independent -> mount ~/build-tools/gradle-7.0.2
#   - sdk_root  : x86_64 binaries inside, but they run under qemu after binfmt
#                 is installed on the host -> mount ~/build-tools/sdk_root
#
# Must be built with: docker build --platform linux/amd64 ...
FROM --platform=linux/amd64 ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# JDK 11 (same as azure-pipelines.yml) + cmake (must be amd64, cannot reuse the
# aarch64 cmake built on the host) + misc tools used by build.sh / CMake.
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        wget \
        unzip \
        zip \
        git \
        pkg-config \
        openjdk-11-jdk-headless \
        cmake \
        ninja-build \
        make \
        udev \
        adb \
    && rm -rf /var/lib/apt/lists/*

ENV JAVA_HOME=/usr/lib/jvm/java-11-openjdk-amd64
ENV PATH=${JAVA_HOME}/bin:/opt/gradle-7.0.2/bin:${PATH}
ENV ANDROID_HOME=/opt/android-sdk
ENV ANDROID_SDK_ROOT=/opt/android-sdk
ENV ANDROID_NDK_HOME=/opt/android-sdk/ndk/23.0.7599858
ENV LANG=C.UTF-8

# Keep Gradle heap modest: this Jetson only has ~7.6GiB shared with the GPU.
ENV GRADLE_OPTS="-Dorg.gradle.jvmargs=-Xmx1500m -XX:MaxMetaspaceSize=512m -Dfile.encoding=UTF-8"

WORKDIR /src
