SUBAGENT = netsvc

pkglib_LTLIBRARIES = netsvc.la
netsvc_la_SOURCES = http.cpp main.cpp other.cpp pop3.cpp smtp.cpp ssh.cpp tcp.cpp telnet.cpp tls.cpp util.cpp
netsvc_la_CPPFLAGS=-I@top_srcdir@/include -I@top_srcdir@/build
netsvc_la_LDFLAGS = -module -avoid-version -export-symbols ../subagent.sym
netsvc_la_LIBADD = ../../libnxagent/libnxagent.la ../../../libnetxms/libnetxms.la

EXTRA_DIST = netsvc.h netsvc.vcxproj netsvc.vcxproj.filters

if !STATIC_BUILD
install-exec-hook:
	if test "x`uname -s`" = "xAIX" ; then OBJECT_MODE=@OBJECT_MODE@ $(AR) x $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ ; rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).a ; fi
	mv -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT)@SHLIB_SUFFIX@ $(DESTDIR)$(pkglibdir)/$(SUBAGENT).nsm
	rm -f $(DESTDIR)$(pkglibdir)/$(SUBAGENT).la
	rm -f $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
	ln -s netxms/$(SUBAGENT).nsm $(DESTDIR)$(libdir)/libnsm_$(SUBAGENT)@SHLIB_SUFFIX@
endif
