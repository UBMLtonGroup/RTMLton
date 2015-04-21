# -*- mode: ruby -*-
# vi: set ft=ruby :

# Vagrantfile API/syntax version. Don't touch unless you know what you're doing!
VAGRANTFILE_API_VERSION = "2"

Vagrant.configure(VAGRANTFILE_API_VERSION) do |config|
  # All Vagrant configuration is done here. The most common configuration
  # options are documented and commented below. For a complete reference,
  # please see the online documentation at vagrantup.com.

  # Every Vagrant virtual environment requires a box to build off of.

  # 64bit (dont use for RTGC)
  #config.vm.box = "ubuntu/trusty64"
  #config.vm.box_url = "https://cloud-images.ubuntu.com/vagrant/trusty/current/trusty-server-cloudimg-x64-vagrant-disk1.box"

  # 32bit (works with RTGC)
  config.vm.box = "trusty32"
  config.vm.box_url = "https://cloud-images.ubuntu.com/vagrant/trusty/current/trusty-server-cloudimg-i386-vagrant-disk1.box"

  # Create a forwarded port mapping which allows access to a specific port
  # within the machine from a port on the host machine. In the example below,
  # accessing "localhost:8080" will access port 80 on the guest machine.
  #config.vm.network :forwarded_port, guest: 80, host: 8080

  # Create a private network, which allows host-only access to the machine
  # using a specific IP.
  # config.vm.network :private_network, ip: "192.168.33.10"

  # Create a public network, which generally matched to bridged network.
  # Bridged networks make the machine appear as another physical device on
  # your network.
  config.vm.network :public_network

  # If true, then any SSH connections made will enable agent forwarding.
  # Default value: false
  config.ssh.forward_agent = true
  config.ssh.forward_x11 = true

  # Share an additional folder to the guest VM. The first argument is
  # the path on the host to the actual folder. The second argument is
  # the path on the guest to mount the folder. And the optional third
  # argument is a set of non-required options.

  config.vm.synced_folder ".", "/vagrant_data"
  if ENV['USER'] == "jcmurphy" then
      config.vm.synced_folder "#{ENV['HOME']}/Documents/Aptana/CSE", "/git"
  end

  # Provider-specific configuration so you can fine-tune various
  # backing providers for Vagrant. These expose provider-specific options.
  # Example for VirtualBox:
  #
  # config.vm.provider :virtualbox do |vb|
  #   # Don't boot with headless mode
  #   vb.gui = true
  #
  #   # Use VBoxManage to customize the VM. For example to change memory:
  #   vb.customize ["modifyvm", :id, "--memory", "1024"]
  # end
  #
  # View the documentation for the provider you're using for more
  # information on available options.

  config.vm.define "rtemsbox1" do |rtemsbox1|
    rtemsbox1.vm.hostname = "rtemsbox1"
    rtemsbox1.vm.provider :virtualbox do |vb|
      vb.customize ["modifyvm", :id, "--memory", "2048"]
      vb.customize ["modifyvm", :id, "--ioapic", "on"]
      vb.customize ["modifyvm", :id, "--cpus", "2"] 
    end
  end

  # Enable provisioning with Puppet stand alone.  Puppet manifests
  # are contained in a directory path relative to this Vagrantfile.
  # You will need to create the manifests directory and a manifest in
  # the file precise64.pp in the manifests_path directory.
  #
  # An example Puppet manifest to provision the message of the day:
  #
   # group { "puppet":
   #   ensure => "present",
   # }
   #
   # File { owner => 0, group => 0, mode => 0644 }
   #
   # file { '/etc/motd':
   #   content => "Welcome to your Vagrant-built virtual machine!
   #               Managed by Puppet.\n"
   # }
  #
  $rootscript = <<SCRIPT
  echo -e "net.ipv6.conf.all.disable_ipv6 = 1\nnet.ipv6.conf.default.disable_ipv6 = 1\nnet.ipv6.conf.lo.disable_ipv6 = 1\n" >> /etc/sysctl.conf
  sysctl -p
  apt-get update -y
  apt-get install -y python-pip build-essential python-dev libffi-dev git bison cvs g++ flex python-dev zlib1g-dev libncurses-dev  unzip automake autoconf qemu gdb
  cd /tmp
  wget http://ftp.gnu.org/gnu/texinfo/texinfo-4.13.tar.gz
  gzip -dc < texinfo-4.13.tar.gz | tar -xf -
  cd texinfo-4.13
  ./configure --prefix=/usr/local
  make
  make install
  apt-get install -y mlton
  echo " " >> /etc/ssh/sshd_config
  echo X11UseLocalhost no >> /etc/ssh/sshd_config
  restart ssh
SCRIPT

  $nonrootscript = <<SCRIPT
  git clone https://github.com/jeffmurphy/rtems-source-builder.git
  mkdir -p ~/development/rtems/4.10
  cd rtems-source-builder/rtems
  ../source-builder/sb-set-builder --log=l-sparc.txt --prefix=$HOME/development/rtems/4.10 --with-rtems 4.10/rtems-sparc
  ../source-builder/sb-set-builder --log=l-i386.txt  --prefix=$HOME/development/rtems/4.10 --with-rtems 4.10/rtems-i386
  cd $HOME
  wget http://www.gaisler.com/anonftp/tsim/tsim-eval-2.0.36.tar.gz
  tar -zxf tsim-eval-2.0.36.tar.gz
  git clone https://github.com/RTEMS/examples-v2.git
  cd examples-v2
  git checkout examples-v2-4-10-branch
SCRIPT

  config.vm.provision "shell", inline: $rootscript
  config.vm.provision "shell", inline: $nonrootscript, privileged: false

end
