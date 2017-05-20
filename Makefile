PACKAGE=a2pi
VERSION=0.2.2
DIST=$(PACKAGE)-$(VERSION)
DISTDIR=./$(DIST)

a2pi:
	$(MAKE) -C src

clean:
	-rm $(PACKAGE)_*
	-rm *.deb
	-rm -rf $(DISTDIR)
	-rm *.tar.gz
	$(MAKE) -C src clean

install:
	$(MAKE) -C src install

dist:
	$(MAKE) clean
	mkdir $(DISTDIR)
	-chmod 777 $(DISTDIR)
	cp LICENSE.txt $(DISTDIR)
	cp README.md $(DISTDIR)
	cp Makefile $(DISTDIR)
	cp -R ./debian $(DISTDIR)
	cp -R ./docs $(DISTDIR)
	cp -R ./share $(DISTDIR)
	cp -R ./src $(DISTDIR)
	-chmod -R a+r $(DISTDIR)
	tar czf $(DIST).tar.gz $(DISTDIR)

deb:
	$(MAKE) dist
	mv $(DIST).tar.gz $(PACKAGE)_$(VERSION).orig.tar.gz
	cd $(DIST); debuild -us -uc
	rm $(PACKAGE)_$(VERSION).orig.tar.gz
