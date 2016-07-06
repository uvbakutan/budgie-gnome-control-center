Workflow:

apt-get source gnome-control-center and gnome-control-center-data

apt-get install checkinstall

apt-get build-dep gnome-control-center-data gnome-control-center

./configure # (inside main directory)
make

sudo checkinstall # (answer questions script asks)

sudo dpkg -i -B --force-depends <package-name>.deb ; sudo  apt-get -f install
