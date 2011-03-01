# 
# This Makefile is authored and maintained by Ben Griffin of Red Snapper Ltd 
# This Makefile is a part of Obyx - see http://www.obyx.org .
# This file is Copyright (C) 2005-2010 Red Snapper Ltd. http://www.redsnapper.net
# Obyx is registered as a trade mark (2483369) in the name of Red Snapper Ltd.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
###############################################################################
#Also RPATH and CGIDIR must have values -e.g.
#export RPATH=/WWW_ROOT 
#export CGIDIR=bin
#make ${RPATH}/${CGIDIR}/obyx.cgi 
#Environment dependent settings (default is set for mysql)
CC_INCLUDES = -I/usr/include/mysql
LIBDIRS     = -L/lib -L/usr/lib
CC_PATH     = /usr/bin/
CC_WARNA    = -Wno-deprecated -Wswitch -Wunused-function -Wunused-label -Wunused-variable -Wunused-value
CC_WARNB    = -Wunknown-pragmas -Wsign-compare -Wnon-virtual-dtor -Woverloaded-virtual -Wformat -Wmissing-braces -Wparentheses
CC_FLAGS    = -x c++ -DALLOW_MYSQL -funsigned-char -fno-asm -Wno-trigraphs -g -O3 -fmessage-length=0 $(CC_WARNA) $(CC_WARNB)

CGIHOME  = $(RPATH)/$(CGIDIR)
CGIHOMEDEV  = $(RPATH)/$(CGIDIR)_dev
ALL_LIBS = -ldl -lrt -lxerces-c -lxqilla -lstdc++
SHELL    = /bin/sh
###############################################################################
# Standard Library Compiler Settings
CC_EXEC         =$(CC_PATH)gcc
CC_COMPILE_FLAG =-c
CC_OBJECT_FLAG  =-o
SPECIAL_CCFLAGS =
#All include directories are mentioned
CC_ARGS      = -fshort-wchar -I /usr/include $(CC_INCLUDES) -I. $(CC_FLAGS) $(CC_DEFINES)
CC_LINKFLAGS = -Wl,--as-needed 
###############################################################################
#commons
FAST_SRCS        =fast/fast.cpp 
HTTPF_SRCS       =httpfetch/httpfetch.cpp httpfetch/httpfetcher.cpp httpfetch/httpfetchleaf.cpp httpfetch/httpfetchheadparser.cpp 
FILE_SRCS        =filing/device.cpp filing/file.cpp filing/path.cpp filing/utils.cpp 
VDB_MYSQLSRCS    =vdb/mysql/mysqlservice.cpp vdb/mysql/mysqlconnection.cpp vdb/mysql/mysqlquery.cpp 
VDB_POSTGRESRCS  =vdb/postgresql/postgresqlservice.cpp vdb/postgresql/postgresqlconnection.cpp vdb/postgresql/postgresqlquery.cpp 
VDB_SRCS         =vdb/servicefactory.cpp vdb/connection.cpp vdb/query.cpp ${VDB_MYSQLSRCS} ${VDB_POSTGRESRCS}
LOG_SRCS         =logger/logger.cpp logger/httplogger.cpp logger/clilogger.cpp
DATE_SRCS        =date/date.cpp 
STRING_SRCS      =string/infix.cpp string/crypto.cpp string/chars.cpp string/comparison.cpp string/convert.cpp \
                  string/fandr.cpp string/translate.cpp string/regex.cpp string/bitwise.cpp
ENV_SRCS         =environment/environment.cpp environment/IInfImageBase.cpp environment/IInfImageTypes.cpp environment/IInfInfo.cpp environment/IInfNetworkGraphics.cpp environment/IInfReader.cpp
XML_SRCS         =xml/manager.cpp xml/xmlerrhandler.cpp xml/xmlparser.cpp xml/xmlrsrchandler.cpp
HEAD_SRCS        =httphead/httphead.cpp
BASE_SRCS        =${HTTPF_SRCS} ${FILE_SRCS} ${ENV_SRCS} ${VDB_SRCS} ${LOG_SRCS} ${STRING_SRCS} ${DATE_SRCS} ${HEAD_SRCS} ${XML_SRCS}
###############################################################################
OBYX_USES =${BASE_SRCS}
OBYX_LIBS =
OBYX_SRCS = \
     fragmentobject.cpp strobject.cpp xmlobject.cpp dataitem.cpp itemstore.cpp pairqueue.cpp \
     obyxelement.cpp function.cpp comparison.cpp instruction.cpp mapping.cpp iteration.cpp \
     iko.cpp output.cpp inputtype.cpp osiapp.cpp osimessage.cpp document.cpp filer.cpp main.cpp 

FOBYX_USES =${BASE_SRCS} ${FAST_SRCS}
FOBYX_LIBS =-lfcgi -lfcgi++
FOBYX_SRCS =${OBYX_SRCS}
###############################################################################
OBJDIR 	  =obj
DEPDIR 	  =dep
###############################################################################
objs      =$(patsubst %.cpp,$(OBJDIR)/$(2)/%.o,$(1))
setobjs   =$(call objs,$($(1)_USES),commons) \
		   $(call objs,$($(1)_SRCS),$(2)) 
###############################################################################
# parm1 = %1_USES,%_SRCS  parm2 is the application directory. 
OBYX_OBJ      = $(call setobjs,OBYX,core)
FOBYX_OBJ     = $(call setobjs,FOBYX,core)
###############################################################################
build     =@echo "Installing $(2)"; $(CC_EXEC) $(1) $(CC_LINKFLAGS) $(LIBDIRS) $($(3)_LIBI) $(CC_LIBS) $(ALL_LIBS) $($(3)_LIBS) $(CC_OBJECT_FLAG) $(2)
###############################################################################
$(CGIHOME)/obyx.cgi        : $(OBYX_OBJ)  ; $(call build,$+,$@,OBYX)
$(CGIHOMEDEV)/obyx.cgi     : $(OBYX_OBJ)  ; $(call build,$+,$@,OBYX)
###############################################################################
$(CGIHOME)/obyx.fcgi       : SPECIAL_CCFLAGS=-DFAST
$(CGIHOME)/obyx.fcgi       : $(FOBYX_OBJ) ; $(call build,$+,$@,FOBYX)
###############################################################################
$(CGIHOMEDEV)/obyx.fcgi    : SPECIAL_CCFLAGS=-DFAST
$(CGIHOMEDEV)/obyx.fcgi    : $(FOBYX_OBJ) ; $(call build,$+,$@,FOBYX)
###############################################################################
.PHONY: clean all test
clean: 
	rm -rf $(OBJDIR) $(DEPDIR)

test:
	echo [$(OBYX_OBJ)]
###############################################################################
#Dependancy management happens below..
###############################################################################
$(OBJDIR)/%.o : %.cpp
	@if [ ! -d $(OBJDIR)/$(dir $*) ]; then mkdir -p $(OBJDIR)/$(dir $*); fi
	@if [ ! -d $(DEPDIR)/$(dir $*) ]; then mkdir -p $(DEPDIR)/$(dir $*); fi
	@echo "Compiling $<"; $(CC_EXEC) $(CC_ARGS) $(SPECIAL_CCFLAGS) $(CC_COMPILE_FLAG) -MD $(CC_OBJECT_FLAG) $@ $<
	@cp $(OBJDIR)/$(dir $*)$(*F).d $(DEPDIR)/$(dir $*)$(*F).P; \
          sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
              -e '/^$$/ d' -e 's/$$/ :/' < $(OBJDIR)/$(dir $*)$(*F).d >> $(DEPDIR)/$(dir $*)$(*F).P; \
          rm -f $(OBJDIR)/$(dir $*)$(*F).d
###############################################################################
#now do the dependencies by the gcc -MD above <-- can the methods below be improved??
-include $(FAST_SRCS:%.cpp=$(DEPDIR)/commons/%.P)
-include $(XML_SRCS:%.cpp=$(DEPDIR)/commons/%.P)
-include $(VDB_SRCS:%.cpp=$(DEPDIR)/commons/%.P)
-include $(FILE_SRCS:%.cpp=$(DEPDIR)/commons/%.P)
-include $(DATE_SRCS:%.cpp=$(DEPDIR)/commons/%.P)
-include $(LOG_SRCS:%.cpp=$(DEPDIR)/commons/%.P)
-include $(ENV_SRCS:%.cpp=$(DEPDIR)/commons/%.P)
-include $(HEAD_SRCS:%.cpp=$(DEPDIR)/commons/%.P)
-include $(STRING_SRCS:%.cpp=$(DEPDIR)/commons/%.P)
#two lines below for each target
-include $(OBYX_USES:%.cpp=$(DEPDIR)/ocommons/%.P)
-include $(OBYX_SRCS:%.cpp=$(DEPDIR)/ocore/%.P)
-include $(FOBYX_USES:%.cpp=$(DEPDIR)/fcommons/%.P)
-include $(FOBYX_SRCS:%.cpp=$(DEPDIR)/fcore/%.P)
###############################################################################
