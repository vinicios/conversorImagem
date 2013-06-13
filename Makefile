PWD:=$(shell pwd)
BUILD=$(PWD)/build

export BUILD

all:
	$(MAKE) -C utils
	$(MAKE) -C src

clean:
	$(MAKE) clean -C utils
	$(MAKE) clean -C src
