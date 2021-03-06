#!/usr/bin/env bash

# Copyright 2018-2019 Banco Bilbao Vizcaya Argentaria, S.A.

# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at

#     http://www.apache.org/licenses/LICENSE-2.0

# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

function _readlink() { (
  # INFO: readlink does not exist on OSX ¯\_(ツ)_/¯
  cd $(dirname $1)
  echo $PWD/$(basename $1)
) }

# Deployment options
CGO_LDFLAGS_ALLOW='.*'
QED="go run $GOPATH/src/github.com/bbva/qed/main.go"

pub=$(_readlink ./config_files)
tdir=$(mktemp -d /tmp/qed_build.XXX)

sign_path=${pub}
cert_path=${pub}

(
cd ${tdir}

if [ ! -f ${node_path} ]; then (
    version=0.17.0
    folder=node_exporter-${version}.linux-amd64
    link=https://github.com/prometheus/node_exporter/releases/download/v${version}/${folder}.tar.gz
    wget -qO- ${link} | tar xvz -C ./
    cp ${folder}/node_exporter ${node_path}
) fi

if [ ! -f ${sign_path} ]; then
    #build shared signing key
    $QED generate signerkeys --path ${sign_path}
fi

if [ ! -f ${sign_path} ]; then
    #build shared signing key
    $QED generate self-signed-cert --path ${cert_path} --host qed.awesome.aws
fi

)

export GOOS=linux
export GOARCH=amd64
#build server binary
go build -o ${pub}/qed ../../
