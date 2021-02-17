.PHONY: clean

COMMON_INCLUDE=../include

all: user-pure user-libnl

user-libnl: user-libnl.c
	# the nl protocol library suite contains multiple libs
	# we want "libnl" (core) and "libnl-gnl" (generic netlink extension)
	# "-3" is the version (latest 3.x)

	# to get -I and -l: execute '$(pkg-config --cflags --libs libnl-genl-3.0)'
	gcc -Wall -Werror -o $@ $+ -I$(COMMON_INCLUDE) -I/usr/include/libnl3  -lnl-3 -lnl-genl-3

user-pure: user-pure.c
	gcc -Wall -Werror -o $@ $+ -I$(COMMON_INCLUDE)

clean:
	rm -rf user user-libnl user-pure