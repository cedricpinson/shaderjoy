FROM docker.io/ubuntu:20.04
MAINTAINER trigrou

# this image works for wasm and native build

# A few notes
# downloads are done in /build directory
# To make the image as small as possible we use the flag --no-install-recommends with apt-get install commands ( it saves us 400MB)
# to avoid to install ubuntu ppa tools we add it manually with the gpg key and echo in the /etc/apt/sources.list.d/ . We needed that to update the git version so it's possible if we bump the ubuntu version it will not be required anymore

# dont forget to remove programs/libs not used on the last line

ENV TZ=Europe/Paris

COPY clang-11.gpg /etc/apt/trusted.gpg.d/

RUN \
  # Avoid being interrupted during build for user input about timezone
  ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone \
  && apt-get -qq update  \
  && apt-get --no-install-recommends -y install ca-certificates \
  && echo "deb http://apt.llvm.org/focal/ llvm-toolchain-focal-11 main" >>/etc/apt/sources.list.d/clang-11-ppa-focal.list \
  && apt-get -qq update \
  && apt-get --no-install-recommends -y install git \
  # Initial deps
  clang-11 clang-format-11 clang-tidy-11 make libcurl4-gnutls-dev libglfw3-dev cmake \
  # Cleanup
  && rm -rf /var/lib/apt
