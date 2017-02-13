#===-- runtime/Runtest/Makefile ----------------------------*- Makefile -*--===#
#
#                     The KLEE Symbolic Virtual Machine
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

LEVEL=../..

USEDLIBS=kleeBasic.a
LIBRARYNAME=kleeRuntest
SHARED_LIBRARY=1
LINK_LIBS_IN_SHARED = 1
DONT_BUILD_RELINKED = 1
NO_PEDANTIC=1
# Increment version appropriately if ABI/API changes, more details:
# http://tldp.org/HOWTO/Program-Library-HOWTO/shared-libraries.html#AEN135
SHARED_VERSION=1.0

include $(LEVEL)/Makefile.common

#LDFLAGS += -Wl,-soname,lib$(LIBRARYNAME)$(SHLIBEXT)
ifeq ($(HOST_OS),Darwin)
    # set dylib internal version number to llvmCore submission number
    ifdef LLVM_SUBMIT_VERSION
        LLVMLibsOptions := $(LLVMLibsOptions) -Wl,-current_version \
                        -Wl,$(SHARED_VERSION) \
                        -Wl,-compatibility_version -Wl,1
    endif
    # Include everything from the .a's into the shared library.
    LLVMLibsOptions    := $(LLVMLibsOptions) -all_load
    # extra options to override libtool defaults
    LLVMLibsOptions    := $(LLVMLibsOptions)  \
                         -Wl,-dead_strip

    # Mac OS X 10.4 and earlier tools do not allow a second -install_name on command line
    DARWIN_VERS := $(shell echo $(TARGET_TRIPLE) | sed 's/.*darwin\([0-9]*\).*/\1/')
    ifneq ($(DARWIN_VERS),8)
       LLVMLibsOptions    := $(LLVMLibsOptions)  \
                            -Wl,-install_name \
                            -Wl,"@rpath/lib$(LIBRARYNAME)$(SHLIBEXT)"
    endif
endif

ifeq ($(HOST_OS), $(filter $(HOST_OS), DragonFly Linux FreeBSD GNU/kFreeBSD OpenBSD GNU Bitrig))
    # Include everything from the .a's into the shared library.
    LLVMLibsOptions := -Wl,--whole-archive $(LLVMLibsOptions) \
                       -Wl,--no-whole-archive
endif

ifeq ($(HOST_OS), $(filter $(HOST_OS), DragonFly Linux FreeBSD GNU/kFreeBSD GNU))
    # Add soname to the library.
    LLVMLibsOptions += -Wl,--soname,lib$(LIBRARYNAME)$(SHLIBEXT).$(SHARED_VERSION)
endif

ifeq ($(HOST_OS), $(filter $(HOST_OS), Linux GNU GNU/kFreeBSD))
  ifeq (-fsanitize=address,$(filter -fsanitize=address,$(CXXFLAGS)))
    # When building with ASan the library will have undefined symbols into
    # ASan's runtime. We want to allow this and not fail the build.
    $(warning Allowing undefined symbols in $(LIBRARYNAME) due to ASan build)
  else
    # Don't allow unresolved symbols.
    LLVMLibsOptions += -Wl,--no-undefined
  endif
endif

ifeq ($(HOST_OS), Linux)
	# HACK: Setup symlinks that `ldconfig` would set up
	# so that libkleeRuntest can be used from the build directory.
	# This is needed to run tests.
	sym_link_name := $(SharedLibDir)/$(SharedPrefix)$(LIBRARYNAME)$(SHLIBEXT).$(SHARED_VERSION)

all:: $(LibName.SO)
	$(Verb) [ ! -e "$(sym_link_name)" ] && ln -s $(LibName.SO) "$(sym_link_name)" || echo ""

endif
