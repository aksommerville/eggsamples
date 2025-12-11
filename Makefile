all:
.SILENT:
.SECONDARY:

ifndef EGG_SDK
  export EGG_SDK:=../egg2
endif

# Everything in this directory that is *not* a project we can make:
NOT_PROJECTS:=Makefile README.md playable playable-v1 genindex.js

PROJECTS:=$(filter-out $(NOT_PROJECTS),$(wildcard *))

all:$(addprefix build_,$(PROJECTS))
$(addprefix build_,$(PROJECTS)):;make --no-print-directory -C$(subst build_,,$@)
clean:;$(foreach P,$(PROJECTS),make --no-print-directory -C$P $@ &&) true

# `make playables` to build everything and copy their HTML out to a permanent location at our root.
# You can't actually run these directly; a server is required. That changed with Egg v2. So now there's "make serve" too.
PLAYABLE_HTMLS:=$(foreach P,$(PROJECTS),playable/$P/index.html)
PLAYABLE_INDEX:=playable/index.html
playables:all $(PLAYABLE_INDEX);for P in $(PROJECTS) ; do echo $$P ; unzip -q -o -d playable/$$P $$P/out/$$P-web.zip || exit 1 ; done
playable/index.html:genindex.js;node genindex.js playable
serve:playables;http-server playable
