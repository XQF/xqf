#! /bin/sh

# please run this script as root user
# please use tabs for indentation (heredocs starting with <<-EOF need tabs)

echo 'Updating package list'
apt-get update -qq

echo 'Intalling qstat, intltool, cmake dependencies'
apt-get install -q -y qstat intltool cmake

# if release is older than vivid (15.04)
if [ "$(lsb_release -sr | cut -c1-2)" -lt '15' ]
then
	echo 'Adding vivid respository'
	<<-EOF cat >> '/etc/apt/sources.list.d/ubuntu-vivid-main.list'
	deb http://archive.ubuntu.com/ubuntu vivid main universe
	EOF

	echo 'Pin vivid repository to disable package installation from this repository by default'
	<<-EOF cat > '/etc/apt/preferences.d/vivid-pinning'
	Package: *
	Pin: release n=vivid
	Pin-Priority: -100
	EOF

	echo 'Adding the vivid repository key'
	apt-key adv --recv-keys --keyserver keyserver.ubuntu.com 3B4FE6ACC0B21F32

	echo 'Updating package list'
	apt-get update -qq

	echo 'Installing libgeoip-dev libminizip-dev gettext dependencies from vivid repository'
	apt-get install -y -q -t vivid libgeoip-dev libminizip-dev gettext
fi

#EOF
