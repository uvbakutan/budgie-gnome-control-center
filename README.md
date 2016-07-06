Workflow:

apt-get source gnome-control-center and gnome-control-center-data

apt-get install checkinstall

apt-get build-dep gnome-control-center-data gnome-control-center

./configure # (inside main directory)
make

sudo checkinstall # (answer questions script asks)

