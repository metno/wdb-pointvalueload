.xml.1:
	$(DOCBOOK) man $< -o `dirname $@`

.xml.5:
	$(DOCBOOK) man $< -o `dirname $@`

MAN_DOCS = doc/man/wdb-pointLoad.xml 

man1_MANS = $(MAN_DOCS:.xml=.1)

EXTRA_DIST += $(MAN_DOCS)
CLEANFILES += $(MAN_DOCS:.xml=.1)
