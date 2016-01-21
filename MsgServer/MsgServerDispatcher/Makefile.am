bin_PROGRAMS=MsgServerDispatcher

AM_CFLAGS=-g -O0 -std=c++11 -Wall -fPIC -D__PTHREADS__ -D__PTHREADS_MUTEXES__
MsgServerDispatcher_LDFLAGS=-static-libgcc

AM_CPPFLAGS=$(AM_CFLAGS)

AM_CPPFLAGS+=-I$(top_srcdir)/ \
			 -I$(top_srcdir)/DRTListener/ \
			 -I$(top_srcdir)/DRTTransfer/

AM_CPPFLAGS+= -I$(top_srcdir)/../MsgServer/MSCommon/MSBase/ \
		   -I$(top_srcdir)/../MsgServer/MSCommon/MSCommon/ \
		   -I$(top_srcdir)/../MsgServer/MSCommon/MSProtocol/ \
		   -I$(top_srcdir)/../MsgServer/MSCommon/MSThirdParty/ \
		   -I$(top_srcdir)/../MsgServer/MSCommon/MSThirdParty/autoref/ \
		   -I$(top_srcdir)/../MsgServer/MSCommon/MSThirdParty/hiredis/ \
		   -I$(top_srcdir)/../MsgServer/MSCommon/MSThirdParty/libhttp/ \
		   -I$(top_srcdir)/../MsgServer/MSCommon/MSThirdParty/RTComm/ \
		   -I$(top_srcdir)/../MsgServer/MSCommon/MSThirdParty/rtklog/

noinst_HEADERS=$(top_srcdir)/../MsgServer/MSCommon/PlatformHeader.h \
			   $(top_srcdir)/../MsgServer/MSCommon/revision.h \
			   $(top_srcdir)/../MsgServer/MSCommon/sigslot.h

MsgServerDispatcher_SOURCES=$(top_srcdir)/DRTListener/DRTModuleListener.cpp \
						   $(top_srcdir)/DRTTransfer/DRTTransfer.cpp \
						   $(top_srcdir)/DRTTransfer/DRTTransferSession.cpp \
						   $(top_srcdir)/DRTConnectionManager.cpp \
						   $(top_srcdir)/DRTMsgQueue.cpp \
						   $(top_srcdir)/main.cpp

MsgServerDispatcher_LDADD=-L$(top_srcdir)/../MsgServer/MSCommonLib/ -lMSCommon -lRtComm -lhiredis -lhttp -lrtklog
MsgServerDispatcher_LDADD+=-lpthread -lrt -ldl