#!/bin/bash

set -e
set -x

export DEBIAN_FRONTEND=noninteractive

apt-get install -y software-properties-common
add-apt-repository universe
apt-get update
apt-get -y upgrade

apt-get install -y $@
