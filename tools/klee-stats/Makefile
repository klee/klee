#===-- tools/klee-stats/Makefile -----------------------*- Makefile -*--===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

LEVEL = ../..

CONFIGDIRNAME = stat-configs
TOOLSCRIPTNAME := klee-stats
TOOLCONFIGNAME := default.config

# Hack to prevent install trying to strip
# symbols from a python script
KEEP_SYMBOLS := 1

include $(LEVEL)/Makefile.common

# FIXME: Move this stuff (to "build" a script) into Makefile.rules.

ToolBuildPath := $(ToolDir)/$(TOOLSCRIPTNAME)
ConfigBuildPath := $(ToolDir)/$(TOOLCONFIGNAME)
ConfigDirPath := $(ToolDir)/$(CONFIGDIRNAME)

all-local:: $(ToolBuildPath) $(ConfigBuildPath) $(ConfigDirPath)

$(ToolBuildPath): $(ToolDir)/.dir

$(ToolBuildPath): $(PROJ_SRC_DIR)/$(TOOLSCRIPTNAME)
	$(Echo) Copying $(BuildMode) script $(TOOLSCRIPTNAME)
	$(Verb) $(CP) -f $(PROJ_SRC_DIR)/$(TOOLSCRIPTNAME) "$@"
	$(Verb) chmod 0755 "$@"

$(ConfigBuildPath): $(PROJ_SRC_DIR)/$(TOOLCONFIGNAME)
	$(Verb) $(CP) -f $(PROJ_SRC_DIR)/$(TOOLCONFIGNAME) "$@"


$(ConfigDirPath):
	$(Echo) Copying config folder $(CONFIGDIRNAME)
	rsync -rupE $(PROJ_SRC_DIR)/$(CONFIGDIRNAME) $(ToolDir)

ifdef NO_INSTALL
install-local::
	$(Echo) Install circumvented with NO_INSTALL
uninstall-local::
	$(Echo) Uninstall circumvented with NO_INSTALL
else
DestTool = $(DESTDIR)$(PROJ_bindir)/$(TOOLSCRIPTNAME)
DestConfig = $(DESTDIR)$(PROJ_bindir)/$(TOOLCONFIGNAME)

install-local:: $(DestTool) $(DestConfig)

$(DestTool): $(ToolBuildPath) $(DESTDIR)$(PROJ_bindir)
	$(Echo) Installing $(BuildMode) $(DestTool)
	$(Verb) $(ProgInstall) $(ToolBuildPath) $(DestTool)

$(DestConfig):  $(ConfigBuildPath)
	$(Verb) $(ProgInstall) $(ConfigBuildPath) $(DestConfig)

uninstall-local::
	$(Echo) Uninstalling $(BuildMode) $(DestTool)
	-$(Verb) $(RM) -f $(DestTool)
endif
