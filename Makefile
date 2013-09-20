PACKAGE=a2pi
VERSION=0.9.0
DIST=$(PACKAGE)-$(VERSION)
DISTDIR=./$(DIST)

a2pi:
	$(MAKE) -C src

dist:
	-rm -rf $(DISTDIR)
	mkdir $(DISTDIR)
	-chmod 777 $(DISTDIR)
	-cp * $(DISTDIR)
	cp -R ./docs $(DISTDIR)
	cp -R ./share $(DISTDIR)
	cp -R ./src $(DISTDIR)
	-chmod -R a+r $(DISTDIR)
	tar czf $(DIST).tar.gz $(DISTDIR)
	rm -rf $(DISTDIR)
