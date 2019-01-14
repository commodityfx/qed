#  Copyright 2018 Banco Bilbao Vizcaya Argentaria, S.A.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

data "aws_ami" "amazon_linux" {
  most_recent = true

  filter {
    name = "name"
    values = ["amzn-ami-hvm-*-x86_64-gp2"]
  }

  filter {
    name = "owner-alias"
    values = ["amazon"]
  }
}

resource "aws_instance" "prometheus" {
  count                       = "1"
  ami                         = "${data.aws_ami.amazon_linux.id}"
  instance_type               = "${var.instance_type}"

  vpc_security_group_ids      = ["${var.vpc_security_group_ids}"]
  subnet_id                   = "${var.subnet_id}"
  associate_public_ip_address = true
  key_name                    = "${var.key_name}"

  root_block_device = [{
    volume_type = "gp2"
    volume_size = "${var.volume_size}"
  }]

  tags {
    Name = "qed-prometheus"
  }


  provisioner "file" {
      source     = "${path.module}/data"
      destination = "${var.path}"

      connection {
        user = "ec2-user"
      }
  }

  provisioner "file" {
      content     = "${var.config}"
      destination = "${var.path}/prometheus.yml"

      connection {
        user = "ec2-user"
      }
  }

  user_data = <<-DATA
  #!/bin/bash

  while [ ! -f ${var.path}/prometheus ]; do
    sleep 1 # INFO: wait until binary exists
  done
  sleep 1

  chmod +x ${var.path}/prometheus
  ${var.path}/prometheus --config-file=${var.path}/prometheus.yml
  DATA
}