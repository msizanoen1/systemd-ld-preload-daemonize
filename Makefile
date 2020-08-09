libsystemd_daemonize.so: systemd-daemonize.c
	cc -O3 -shared -fPIC -o libsystemd_daemonize.so systemd-daemonize.c `pkg-config --cflags --libs libsystemd` -ldl -g

test: test.c
	cc -o test test.c

clean:
	rm -rf *.so test

.PHONY: clean
