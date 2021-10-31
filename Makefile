#
# Makefile des sources du projet.
#

#
# Organisation des sources.
#

# Packages.
PACKAGES = binScanner scales tourManager uiBenno comBenno clockMaker watchdog

# Deux niveaux de packages sont accessibles.
SRC  = $(wildcard */*.c)		
SRC += $(wildcard */*/*.c)

OBJ = $(SRC:.c=.o)

# Point d'entrée du programme.
MAIN = main.c

# Gestion automatique des dépendances.
DEP = $(MAIN:.c=.d)

# Executable à générer
EXEC = ../$(PROG)

# Inclusion depuis le niveau du package.
CCFLAGS += -I.
CCFLAGS += -DDEBUG # avec debuggage : -DDEBUG | sans debuggage : -DNDEBUG

#
# Règles du Makefile.
#

# Compilation.
all:
	@for p in $(PACKAGES); do (cd $$p && $(MAKE) $@); done
	@$(MAKE) $(EXEC)

$(EXEC): $(OBJ) $(MAIN)
	$(CC) $(CCFLAGS) $(OBJ) $(MAIN) -MF $(DEP) -o $(EXEC) $(LDFLAGS)

# Nettoyage.
.PHONY: clean

clean:
	@for p in $(PACKAGES); do (cd $$p && $(MAKE) $@); done
	@rm -f $(DEP)

-include $(DEP)

