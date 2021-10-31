EXEC = exercice.out

CC = gcc
LD = gcc

CCFLAGS = -Wall
LDFLAGS = -lbluetooth -ljson-c

RM = rm -f

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
# Gestion automatique des dépendances.
DEP = $(SRC:.c=.d)

# Compilation rules
all: $(EXEC)

$(EXEC): $(OBJ)
	$(LD) $^ -o $@ $(LDFLAGS)

# Définition de macro avec -D_...
.c.o:
	$(CC) $(CCFLAGS) -c $< -o $@

# Nettoyage.
.PHONY: clean

# Clean the project
clean:
	$(RM) *.o *.json $(EXEC)

-include $(DEP)
