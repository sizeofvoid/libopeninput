#!/bin/bash

# build container

set -o xtrace

TAG=$1

cat > /etc/containers/storage.conf <<EOF
[storage]
driver = "vfs"
EOF

buildcntr1=$(buildah from --quiet golang:alpine)
buildmnt1=$(buildah mount $buildcntr1)

buildah run $buildcntr1 apk add --update \
                                --no-cache \
                                --repository http://dl-3.alpinelinux.org/alpine/edge/testing/ \
                                bash git make gcc musl-dev glib-dev ostree-dev \
                                bats bzip2 python3-dev \
                                gpgme-dev linux-headers btrfs-progs-dev \
                                libselinux-dev lvm2-dev libseccomp-dev

# build runc
buildah run $buildcntr1 go get github.com/opencontainers/runc
buildah config --workingdir /go/src/github.com/opencontainers/runc/ $buildcntr1
buildah run $buildcntr1 bash -c 'make'
buildah run $buildcntr1 bash -c 'make install'

# build skopeo
buildah run $buildcntr1 git clone --depth 1 --branch master https://github.com/containers/skopeo /go/src/github.com/containers/skopeo
buildah config --workingdir /go/src/github.com/containers/skopeo/ $buildcntr1
buildah run $buildcntr1 bash -c 'make binary-local'

# build libpod
buildah run $buildcntr1 git clone --depth 1 --branch master https://github.com/containers/libpod /go/src/github.com/containers/libpod
buildah config --workingdir /go/src/github.com/containers/libpod/ $buildcntr1
buildah run $buildcntr1 bash -c 'make install.tools'
buildah run $buildcntr1 bash -c 'make'
buildah run $buildcntr1 bash -c 'make install'

# build buildah
buildah run $buildcntr1 git clone --depth 1 --branch master https://github.com/containers/buildah /go/src/github.com/containers/buildah
buildah config --workingdir /go/src/github.com/containers/buildah/ $buildcntr1
buildah run $buildcntr1 bash -c 'make install.tools'
buildah run $buildcntr1 bash -c 'make'
buildah run $buildcntr1 bash -c 'make install'

# build conmon
buildah run $buildcntr1 git clone --depth 1 --branch master https://github.com/kubernetes-sigs/cri-o /go/src/github.com/kubernetes-sigs/cri-o
buildah config --workingdir /go/src/github.com/kubernetes-sigs/cri-o/ $buildcntr1
buildah run $buildcntr1 bash -c 'make install.tools'
buildah run $buildcntr1 bash -c 'make'
buildah run $buildcntr1 bash -c 'make install'


buildcntr2=$(buildah from --quiet alpine:latest)
buildmnt2=$(buildah mount $buildcntr2)
buildah run $buildcntr2 apk add --update \
                                --no-cache \
                                --repository http://dl-3.alpinelinux.org/alpine/edge/testing/ \
                                bash jq curl glib gpgme ostree lvm2 libselinux libseccomp \
                                iptables ip6tables
cp $buildmnt1/usr/local/sbin/runc $buildmnt2/usr/sbin/runc
cp $buildmnt1/go/src/github.com/containers/skopeo/skopeo $buildmnt2/usr/bin/skopeo
cp $buildmnt1/usr/local/bin/podman $buildmnt2/usr/bin/podman
cp $buildmnt1/usr/local/bin/buildah $buildmnt2/usr/bin/buildah
cp $buildmnt1/usr/local/bin/crio $buildmnt2/usr/bin/crio
mkdir $buildmnt2/usr/libexec/crio
cp $buildmnt1/usr/local/libexec/crio/conmon $buildmnt2/usr/libexec/crio/conmon
cp $buildmnt1/usr/local/libexec/crio/pause $buildmnt2/usr/libexec/crio/pause

mkdir $buildmnt2/etc/containers

cat > $buildmnt2/etc/containers/registries.conf <<EOF
# This is a system-wide configuration file used to
# keep track of registries for various container backends.
# It adheres to TOML format and does not support recursive
# lists of registries.

# The default location for this configuration file is /etc/containers/registries.conf.

# The only valid categories are: 'registries.search', 'registries.insecure',
# and 'registries.block'.

[registries.search]
registries = ['docker.io', 'registry.fedoraproject.org', 'quay.io', 'registry.centos.org']

# If you need to access insecure registries, add the registry's fully-qualified name.
# An insecure registry is one that does not have a valid SSL certificate or only does HTTP.
[registries.insecure]
registries = []


# If you need to block pull access from a registry, uncomment the section below
# and add the registries fully-qualified name.
#
# Docker only
[registries.block]
registries = []
EOF

cat > $buildmnt2/etc/containers/policy.json <<EOF
{
    "default": [
        {
            "type": "insecureAcceptAnything"
        }
    ],
    "transports":
        {
            "docker-daemon":
                {
                    "": [{"type":"insecureAcceptAnything"}]
                }
        }
}
EOF

cat > $buildmnt2/etc/containers/storage.conf <<EOF
# This file is is the configuration file for all tools
# that use the containers/storage library.
# See man 5 containers-storage.conf for more information
# The "container storage" table contains all of the server options.
[storage]

# Default Storage Driver
driver = "vfs"
EOF

buildah unmount $buildcntr2
buildah commit --quiet $buildcntr2 $TAG

#clean up build

buildah rm $buildcntr1 $buildcntr2
