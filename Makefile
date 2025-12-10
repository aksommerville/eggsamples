all:
.SILENT:
.SECONDARY:

export EGG_SDK:=../egg2

# Everything in this directory that is *not* a project we can make:
NOT_PROJECTS:=Makefile README.md playable genindex.js

PROJECTS:=$(filter-out $(NOT_PROJECTS),$(wildcard *))

all:$(addprefix build_,$(PROJECTS))
$(addprefix build_,$(PROJECTS)):;make --no-print-directory -C$(subst build_,,$@)
clean:;$(foreach P,$(PROJECTS),make --no-print-directory -C$P $@ &&) true

# `make playables` to build everything and copy their HTML out to a permanent location at our root.
playables:all playables/index.html;for P in $(PROJECTS) ; do cp $$P/out/$$P.html playable/$$P.html || exit 1 ; echo "  playable/$$P.html" ; done
playables/index.html:genindex.js $(patsubst playable/%.html,%,$(PROJECTS));node genindex.js playable
