CC=aarch64-linux-android-gcc # android target
#CC=aarch64-linux-gnu-gcc # e.g., linux target
CFLAGS=-pie

.PHONY: clean

executor: executor.c

clean: executor
	$(RM) $^
