# -*- mode: ruby -*-
# vi: set ft=ruby :

Vagrant.configure("2") do |config|

  config.vm.box = "hashicorp/bionic64"
    
  config.vm.provider "virtualbox" do |v|
    v.name = "fdim-controller platformio VM"
    v.customize ["modifyvm", :id, "--usb", "on"]
    v.customize ["modifyvm", :id, "--usbehci", "on"]
    v.customize ["usbfilter", "add", "0",
                 "--target", :id,
                 "--name", "SparkFun Pro Micro",
                 "--vendorid", "1b4f",
                 "--productid", "9206",
                 "--remote", "no"]
    v.customize ["usbfilter", "add", "1",
                 "--target", :id,
                 "--name", "SparkFun Pro Micro",
                 "--vendorid", "1b4f",
                 "--productid", "9203",
                 "--remote", "no"]
    v.customize ["usbfilter", "add", "2",
                 "--target", :id,
                 "--name", "Arduino LLC Arduino Leonardo [0001]",
                 "--vendorid", "2341",
                 "--productid", "0036",
                 "--remote", "no"]
  end

  config.vm.provision "shell", privileged: false, inline: <<-SHELL 
    sudo -H DEBIAN_FRONTEND=noninteractive apt-get update
    sudo -H DEBIAN_FRONTEND=noninteractive apt-get -yq install python-pip git
    pip install -U platformio
    curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core/master/scripts/99-platformio-udev.rules | sudo -H tee /etc/udev/rules.d/99-platformio-udev.rules
    sudo -H service udev restart

    if [ ! -d ~/fdim-controller ] ; then
      cd ~
      git clone https://github.com/p1ne/fdim-controller.git
      cd fdim-controller
      git submodule init
      cd ..
    fi
    cd fdim-controller
    git pull
    git submodule update

    sudo ~/.local/bin/platformio run --target upload
   SHELL
end
