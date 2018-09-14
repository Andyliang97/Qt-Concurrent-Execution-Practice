#!/bin/bash

clear
clear

export CLIBS=$PWD
export LD_LIBRARY_PATH=$PWD

export NULL=/dev/null

pushd message >& $NULL
	make distclean
	qmake
	make
popd >& $NULL

pushd external >& $NULL
	make distclean
	qmake
	make
popd >& $NULL

pushd external >& $NULL
	gnome-terminal -x bash -c "./external 6 600; echo done; read -p \"\""
	gnome-terminal -x bash -c "./external 5 500; echo done; read -p \"\""
	gnome-terminal -x bash -c "./external 4 400; echo done; read -p \"\""
	gnome-terminal -x bash -c "./external 3 300; echo done; read -p \"\""
	gnome-terminal -x bash -c "./external 2 200; echo done; read -p \"\""
	gnome-terminal -x bash -c "./external 1 100; echo done; read -p \"\""
	sleep 1s
	gnome-terminal -x bash -c "./external 0   0; echo done; read -p \"\""
popd >& $NULL
