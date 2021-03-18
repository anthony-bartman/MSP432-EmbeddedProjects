# invoke SourceDir generated makefile for event.pem4f
event.pem4f: .libraries,event.pem4f
.libraries,event.pem4f: package/cfg/event_pem4f.xdl
	$(MAKE) -f C:\advembed_labs\bartmanLab6/src/makefile.libs

clean::
	$(MAKE) -f C:\advembed_labs\bartmanLab6/src/makefile.libs clean

