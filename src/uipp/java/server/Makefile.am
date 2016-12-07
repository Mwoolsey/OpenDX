srvbindir = $(prefix)/dx/java/server/bin
srvoutputdir = $(prefix)/dx/java/output
userhtmldir = $(prefix)/dx/java/user
# the server script (in srvbindir) will actually cd and run from the class subdir so that java will find the classes
# the server will expect to find a file dxserver.paths there to tell the exec where to find .net files
srvrundir = $(srvbindir)/../class
srvrun_DATA = dxserver.paths
srvoutput_DATA = README.output
srvbin_SCRIPTS = $(SCRFILES)

SCRFILES = stopserver startserver startserver.bat dxserver.paths

DISTCLEANFILES = README.output server_DXServer.h server_DXServerThread.h

EXTRA_DIST = CleanUpDaemon.java ConnectThread.java ConnectionEntry.java \
	DXLink.c DXServer.java DXServerThread.java DXThread.java \
	DXThreadCommand.java FileStyle.java FileStyleANY.java \
	FileStyleHTM.java FileStyleHTML.java FileStyleWRAP.java \
	ServerCommand.java ServerThreadCommand.java \
	StatusThread.java StatusThreadCommand.java $(SCRFILES) 


SUBDIRS = macros

README.output:
	echo "A directory is placed at the same level as " > README.output
	echo "htmlpages for default output.  Make this " >> README.output
	echo "directory writable!" >> README.output

install-exec-local:
	$(mkinstalldirs) $(DESTDIR)$(srvoutputdir)
	$(mkinstalldirs) $(DESTDIR)$(userhtmldir)
	chmod 777 $(DESTDIR)$(srvoutputdir)
	chmod 777 $(DESTDIR)$(userhtmldir)
