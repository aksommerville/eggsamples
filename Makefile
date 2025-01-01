all:
.SILENT:
.SECONDARY:

# Everything in this directory that is *not* a project we can make:
NOT_PROJECTS:=Makefile README.md playable

PROJECTS:=$(filter-out $(NOT_PROJECTS),$(wildcard *))

all:$(addprefix build_,$(PROJECTS))
$(addprefix build_,$(PROJECTS)):;make --no-print-directory -C$(subst build_,,$@)
clean:;$(foreach P,$(PROJECTS),make --no-print-directory -C$P $@ &&) true

# `make playables` to build everything and copy their HTML out to a permanent location at our root.
playables:all;for P in $(PROJECTS) ; do cp $$P/out/$$P.html playable/$$P.html || exit 1 ; echo "  playable/$$P.html" ; done
