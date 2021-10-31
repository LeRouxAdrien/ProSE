#
# Monix C - Makefile du package modele des sources.
#
# @author Matthias Brun
#

#
# Organisation des sources.
#
# Sous packages.
PACKAGES = proxyTourDirector dispatcherBenno postmanBenno

#
# Règles du Makefile.
#

# Compilation.
all:
	@for p in $(PACKAGES); do (cd $$p && $(MAKE) $@); done

# Nettoyage.
.PHONY: clean

clean:
	@for p in $(PACKAGES); do (cd $$p && $(MAKE) $@); done

