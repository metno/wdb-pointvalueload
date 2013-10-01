.xml.html:
	$(DOCBOOK) html-nochunks $< -o `dirname $@`

DOCUMENTATION_SRC = \
	doc/developer/pointload_system-design-specification.xml \
	doc/developer/pointload_system-test-specification.xml

DOCUMENTATION_OUTPUT = $(DOCUMENTATION_SRC:.xml=.html)
	
html_DATA = $(DOCUMENTATION_OUTPUT)

documentation:	$(DOCUMENTATION_OUTPUT)

UNUSED_DOC = \
	doc/user/pointLoad.man.xml

USED_DOCS = \
	doc/docbook/wdb_entities.ent \
	doc/docbook/xinclude.mod
	
EXTRA_DIST += $(DOCUMENTATION_SRC) $(USED_DOCS)

CLEANFILES += $(DOCUMENTATION_OUTPUT)

include doc/doxygen/doxygen.mk
