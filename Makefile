TARGET = npshell

INCDIR = include
SRCDIR = src

CC = gcc
CFLAGS = -std=gnu11 -Wall -I $(INCDIR) -I $(SRCDIR)

SOURCES := $(wildcard $(SRCDIR)/*.c)

RM = rm -f

$(TARGET):
	@echo "Compiling" $@ "..."
	$(CC) $(CFLAGS) $(SOURCES) -o $@


.PHONY: remake
remake: remove $(TARGET)
	@echo "Remake done."

.PHONY: remove
remove: 
	@$(RM) $(TARGET)

