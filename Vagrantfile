# -*- mode: ruby -*-
# vi: set ft=ruby :

# All Vagrant configuration is done below. The "2" in Vagrant.configure
# configures the configuration version (we support older styles for
# backwards compatibility). Please don't change it unless you know what
# you're doing.
Vagrant.configure(2) do |config|
  # The most common configuration options are documented and commented below.
  # For a complete reference, please see the online documentation at
  # https://docs.vagrantup.com.

  # Every Vagrant development environment requires a box. You can search for
  # boxes at https://atlas.hashicorp.com/search.
  config.vm.box = "ubuntu/trusty64"

  # Disable automatic box update checking. If you disable this, then
  # boxes will only be checked for updates when the user runs
  # `vagrant box outdated`. This is not recommended.
  # config.vm.box_check_update = false

  # Create a forwarded port mapping which allows access to a specific port
  # within the machine from a port on the host machine. In the example below,
  # accessing "localhost:8080" will access port 80 on the guest machine.
  # config.vm.network "forwarded_port", guest: 80, host: 8080

  # Create a private network, which allows host-only access to the machine
  # using a specific IP.
  # config.vm.network "private_network", ip: "192.168.33.10"

  # Create a public network, which generally matched to bridged network.
  # Bridged networks make the machine appear as another physical device on
  # your network.
  # config.vm.network "public_network"

  # Share an additional folder to the guest VM. The first argument is
  # the path on the host to the actual folder. The second argument is
  # the path on the guest to mount the folder. And the optional third
  # argument is a set of non-required options.
  # config.vm.synced_folder "../data", "/vagrant_data"

  # Provider-specific configuration so you can fine-tune various
  # backing providers for Vagrant. These expose provider-specific options.
  # Example for VirtualBox:
  #
  config.vm.provider "virtualbox" do |vb|
    vb.gui = false

    # Some C++ files take 1600+ MiB RSS
    vb.memory = "2048"

    # No need to rush, as if multiple compilation, you'll even need more RSS
    vb.cpus = 1
  end
  #
  # View the documentation for the provider you are using for more
  # information on available options.

  # Define a Vagrant Push strategy for pushing to Atlas. Other push strategies
  # such as FTP and Heroku are also available. See the documentation at
  # https://docs.vagrantup.com/v2/push/atlas.html for more information.
  # config.push.define "atlas" do |push|
  #   push.app = "YOUR_ATLAS_USERNAME/YOUR_APPLICATION_NAME"
  # end

  # Enable provisioning with a shell script. Additional provisioners such as
  # Puppet, Chef, Ansible, Salt, and Docker are also available. Please see the
  # documentation for more information about their specific syntax and use.
  config.vm.provision "shell",
	privileged: false,
	inline: <<-SHELL

	# Remove the default puppet
	sudo apt remove --purge ruby

	# Setup the OS, as the .travis.yml
	# XXX - should be able to parse the file, and extract important things


	sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y
	sudo add-apt-repository ppa:george-edison55/cmake-3.x -y
	sudo add-apt-repository "deb http://llvm.org/apt/trusty/ llvm-toolchain-trusty-3.9 main" -y
	wget -O - http://llvm.org/apt/llvm-snapshot.gpg.key | sudo apt-key add -
	sudo apt-get update


	sudo apt-get install ccache g++-5 -y
	export CXX="g++-5" CC="gcc-5"
	sudo apt-get install libunwind8-dev libsdl2-dev libboost-locale-dev libboost-filesystem-dev libboost-program-options-dev -y
	mkdir ~/dependency-prefix
	export PKG_CONFIG_PATH=~/dependency-prefix/lib/pkgconfig
	# setup some default settings
	export LSAN_OPTIONS="exitcode=0"
	export NUM_CORES=$(grep '^processor' /proc/cpuinfo|wc -l)

	pushd ./dependencies/glm && cmake -DCMAKE_INSTALL_PREFIX=~/dependency-prefix . > /dev/null && make -j2 > /dev/null && make install > /dev/null && popd

    # Install X11 + OpenGL
    sudo apt-get install -y mesa-utils xinit i3 x11-xserver-utils

    # Install dev env for OpenApoc
    sudo apt-get install -y libsdl2-dev cmake build-essential git libunwind8-dev libboost-locale-dev libboost-filesystem-dev libboost-system-dev
    sudo apt-get install -y gettext libboost-program-options-dev libxml2-utils

	# Install perf tools
	sudo apt-get install -y perf pstack

    unset LC_CTYPE

    git clone /vagrant OpenApoc
    for i in $(cd /vagrant/dependencies && find * -maxdepth 0 -type d)
    do
	git clone /vagrant/dependencies/$i OpenApoc/dependencies/$i
    done
    ( cd OpenApoc && git submodule init && git submodule update )
    ( cd OpenApoc/dependencies/glm && cmake . && make && sudo make install)

    ( ln -s /vagrant/data/XCOM.* OpenApoc/data )
    ( ln -s XCOM.cue OpenApoc/data/cd.iso )

    (
	mkdir -p OpenApoc/build
	cd OpenApoc/build
	cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
	make
    )

  SHELL
end
