.PHONY: all clean

all: patcher.elf launcher.elf

clean:
	rm patcher.elf launcher.elf
	$(MAKE) -C patcher clean
	$(MAKE) -C launcher clean

# OSDSYS Patcher
patcher.elf:
	$(MAKE) -C patcher/$<
	cp patcher/patcher.elf patcher.elf

# Launcher
launcher.elf:
	$(MAKE) -C launcher/$<
	cp launcher/launcher.elf launcher.elf

