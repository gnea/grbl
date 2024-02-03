# This Makefile launches compilation inside of Docker.

# symbolic targets:
all:	grbl.hex

grbl.hex: 
	docker run --rm -it -v "${PWD}":/source tiryoh/grbl-builder:latest make --makefile Makefile.compilation

clean:
	rm -f grbl.hex $(BUILDDIR)/*.o $(BUILDDIR)/*.d $(BUILDDIR)/*.elf
