/*****************************************************************************
 * Project: RooFit                                                           *
 * Package: RooFitCore                                                       *
 * @(#)root/roofitcore:$Id$
 * Authors:                                                                  *
 *   WV, Wouter Verkerke, UC Santa Barbara, verkerke@slac.stanford.edu       *
 *   DK, David Kirkby,    UC Irvine,         dkirkby@uci.edu                 *
 *                                                                           *
 * Copyright (c) 2000-2005, Regents of the University of California          *
 *                          and Stanford University. All rights reserved.    *
 *                                                                           *
 * Redistribution and use in source and binary forms,                        *
 * with or without modification, are permitted according to the terms        *
 * listed in LICENSE (http://roofit.sourceforge.net/license.txt)             *
 *****************************************************************************/

// -- CLASS DESCRIPTION [MISC] --
//
// The RooMsgService class is a singleton class that organizes informational, debugging, 
// warning and errors messages generated by the RooFit core code. 
//
// Each message generated by the core
// has a message level (DEBUG,INFO,PROGRESS,WARNING,ERROR or FATAL), an originating object,
// and a 'topic'. Currently implemented topics are "Generation","Plotting",
// "Integration", "Minimization" and "Workspace" and "ChangeTracking".
//
// The RooMsgService class allows to filter and redirect messages into 'streams' 
// according to message level, topic, (base) class of originating object, name of originating
// object and based on attribute labels attached to individual objects. 
// 
// The current default configuration creates streams for all messages at WARNING level
// or higher (e.g. ERROR and FATAL) and for all INFO message on topics Generation,Plotting,
// Integration and Minimization and redirects them to stdout. Users can create additional streams 
// for logging of e.g. DEBUG messages on particular topics or objects and or redirect streams to
// C++ streams or files.
// 
// The singleton instance is accessible through RooMsgService::instance() ;
//

#define INST_MSG_SERVICE

#include <sys/types.h>

#include "RooFit.h"
#include "RooAbsArg.h"
#include "TClass.h"
#include "TROOT.h"

#include "RooMsgService.h"
#include "RooCmdArg.h"
#include "RooCmdConfig.h"
#include "RooGlobalFunc.h"
#include "RooSentinel.h"

#include "Riostream.h"
#include <iomanip>
#include <fstream>
using namespace std ;
using namespace RooFit ;

ClassImp(RooMsgService)
;

RooMsgService* RooMsgService::_instance = 0 ;
Int_t RooMsgService::_debugCount = 0 ;

void RooMsgService::cleanup() 
{
  if (_instance) {
    delete _instance ;
    _instance = 0 ;
  }
}


RooMsgService::RooMsgService() 
{
  // Constructor
  _silentMode = kFALSE ;
  _showPid = kFALSE ;
  _globMinLevel = DEBUG ;

  _devnull = new ofstream("/dev/null") ;

  _levelNames[DEBUG]="DEBUG" ;
  _levelNames[INFO]="INFO" ;
  _levelNames[PROGRESS]="PROGRESS" ;
  _levelNames[WARNING]="WARNING" ;
  _levelNames[ERROR]="ERROR" ;
  _levelNames[FATAL]="FATAL" ;

  _topicNames[Generation]="Generation" ;
  _topicNames[Minimization]="Minization" ;
  _topicNames[Plotting]="Plotting" ;
  _topicNames[Fitting]="Fitting" ;
  _topicNames[Integration]="Integration" ;
  _topicNames[LinkStateMgmt]="LinkStateMgmt" ;
  _topicNames[Eval]="Eval" ;
  _topicNames[Caching]="Caching" ;
  _topicNames[Optimization]="Optimization" ;
  _topicNames[ObjectHandling]="ObjectHandling" ;
  _topicNames[InputArguments]="InputArguments" ;
  _topicNames[Tracing]="Tracing" ;

  _instance = this ;
  gMsgService = this ;

  addStream(RooMsgService::WARNING) ;
  addStream(RooMsgService::INFO,Topic(RooMsgService::Generation|RooMsgService::Plotting|RooMsgService::Fitting|RooMsgService::Optimization|RooMsgService::Minimization|RooMsgService::Caching|RooMsgService::ObjectHandling)) ;


}

RooMsgService::~RooMsgService() 
{
  // Destructor

  // Delete all ostreams we own ;

  map<string,ostream*>::iterator iter = _files.begin() ;
  for (; iter != _files.end() ; ++iter) {
    delete iter->second ;
  }

  delete _devnull ;
}


Bool_t RooMsgService::anyDebug() 
{ 
  return instance()._debugCount>0 ; 
}


Int_t RooMsgService::addStream(MsgLevel level, const RooCmdArg& arg1, const RooCmdArg& arg2, const RooCmdArg& arg3, 
    	      			               const RooCmdArg& arg4, const RooCmdArg& arg5, const RooCmdArg& arg6) 
{
  // Add a message logging stream for message with given MsgLevel or higher (i.e. more severe)
  // This method accepts the following arguments to configure the stream
  //
  // Output Style options
  // --------------------
  // Prefix(Bool_t flag=kTRUE) -- Prefix all messages in this stream with Topic/Originator information
  //
  // Filtering options
  // -----------------
  // Topic(const char*)        -- Restrict stream to messages on given topic
  // ObjectName(const char*)   -- Restrict stream to messages from object with given name
  // ClassName(const char*)    -- Restrict stream to messages from objects with given class name
  // BaseClassName(const char*)-- Restrict stream to messages from objects with given base class name
  // LabelName(const chat*)    -- Restrict stream to messages from objects setAtrribute(const char*) tag with given name
  //
  // Output redirection options
  // --------------------------
  // OutputFile(const char*)  -- Send output to file with given name. Multiple streams can write to same file.
  // OutputStream(ostream&)   -- Send output to given C++ stream. Multiple message streams can write to same c++ stream
  //
  // The return value is the unique ID code of the defined stream


  // Aggregate all arguments in a list
  RooLinkedList l ;
  l.Add((TObject*)&arg1) ;  l.Add((TObject*)&arg2) ;  
  l.Add((TObject*)&arg3) ;  l.Add((TObject*)&arg4) ;
  l.Add((TObject*)&arg5) ;  l.Add((TObject*)&arg6) ;  

  // Define configuration for this method
  RooCmdConfig pc(Form("RooMsgService::addReportingStream(%s)",GetName())) ;
  pc.defineInt("prefix","Prefix",0,kTRUE) ;
  pc.defineInt("color","Color",0,static_cast<Int_t>(kBlack)) ;
  pc.defineInt("topic","Topic",0,0xFFFFF) ;
  pc.defineString("objName","ObjectName",0,"") ;
  pc.defineString("className","ClassName",0,"") ;
  pc.defineString("baseClassName","BaseClassName",0,"") ;
  pc.defineString("tagName","LabelName",0,"") ;
  pc.defineString("outFile","OutputFile",0,"") ;
  pc.defineObject("outStream","OutputStream",0,0) ;
  pc.defineMutex("OutputFile","OutputStream") ;

  // Process & check varargs 
  pc.process(l) ;
  if (!pc.ok(kTRUE)) {
    return -1 ;
  }

  // Extract values from named arguments
  MsgTopic topic =  (MsgTopic) pc.getInt("topic") ;
  const char* objName =  pc.getString("objName") ;
  const char* className =  pc.getString("className") ;
  const char* baseClassName =  pc.getString("baseClassName") ;
  const char* tagName =  pc.getString("tagName") ;
  const char* outFile = pc.getString("outFile") ;
  Bool_t prefix = pc.getInt("prefix") ;
  Color_t color = static_cast<Color_t>(pc.getInt("color")) ;
  ostream* os = reinterpret_cast<ostream*>(pc.getObject("outStream")) ;

  // Create new stream object
  StreamConfig newStream ;

  // Store configuration info
  newStream.active = kTRUE ;
  newStream.minLevel = level ;
  newStream.topic = topic ;
  newStream.objectName = (objName ? objName : "" ) ;
  newStream.className = (className ? className : "" ) ;
  newStream.baseClassName = (baseClassName ? baseClassName : "" ) ;
  newStream.tagName = (tagName ? tagName : "" ) ;
  newStream.color = color ;
  newStream.prefix = prefix ;
  newStream.universal = (newStream.objectName=="" && newStream.className=="" && newStream.baseClassName=="" && newStream.tagName=="") ;

  // Update debug stream count 
  if (level==DEBUG) {
    _debugCount++ ;
  }

  // Configure output
  if (os) {

    // To given non-owned stream
    newStream.os = os ;

  } else if (string(outFile).size()>0) {

    // See if we already opened the file
    ostream* os2 = _files["outFile"] ;

    if (!os2) {

      // To given file name, create owned stream for it
      os2 = new ofstream(outFile) ;

      if (!*os2) {
	cout << "RooMsgService::addReportingStream ERROR: cannot open output log file " << outFile << " reverting stream to stdout" << endl ;
	delete os2 ;
	newStream.os = &cout ;
      }
    }
    _files["outFile"] = os2 ;
        
  } else {

    // To stdout
    newStream.os = &cout ;

  }


  // Add it to list of active streams ;
  _streams.push_back(newStream) ;

  // Return stream identifier
  return _streams.size()-1 ;
}



void RooMsgService::deleteStream(Int_t id) 
{
  // Delete stream with given unique ID code

  vector<StreamConfig>::iterator iter = _streams.begin() ;
  iter += id ;

  // Update debug stream count 
  if (iter->minLevel==DEBUG) {
    _debugCount-- ;
  }

  _streams.erase(iter) ;
}


void RooMsgService::setStreamStatus(Int_t id, Bool_t flag) 
{
  // (De)Activate stream with given unique ID

  if (id<0 || id>=static_cast<Int_t>(_streams.size())) {
    cout << "RooMsgService::setStreamStatus() ERROR: invalid stream ID " << id << endl ;
    return ;
  }

  // Update debug stream count 
  if (_streams[id].minLevel==DEBUG) {
    _debugCount += flag ? 1 : -1 ;
  }

  _streams[id].active = flag ;
}


Bool_t RooMsgService::getStreamStatus(Int_t id) const 
{
  // Get activation status of stream with given unique ID

  if (id<0 || id>= static_cast<Int_t>(_streams.size())) {
    cout << "RooMsgService::getStreamStatus() ERROR: invalid stream ID " << id << endl ;
    return kFALSE ;
  }
  return _streams[id].active ;
}


RooMsgService& RooMsgService::instance() 
{
  // Return reference to singleton instance 
  if (!_instance) {
    new RooMsgService() ;    
    RooSentinel::activate() ;
  }
  return *_instance ;
}


Bool_t RooMsgService::isActive(const RooAbsArg* self, MsgTopic topic, MsgLevel level) 
{
  // Check if logging is active for given object/topic/MsgLevel combination
  return (activeStream(self,topic,level)>=0) ;
}

Bool_t RooMsgService::isActive(const TObject* self, MsgTopic topic, MsgLevel level) 
{
  // Check if logging is active for given object/topic/MsgLevel combination
  return (activeStream(self,topic,level)>=0) ;
}

Int_t RooMsgService::activeStream(const RooAbsArg* self, MsgTopic topic, MsgLevel level) 
{
  // Find appropriate logging stream for message from given object with given topic and message level
  if (level<_globMinLevel) return -1 ;
  for (UInt_t i=0 ; i<_streams.size() ; i++) {
    if (_streams[i].match(level,topic,self)) {
      return i ;
    }
  }
  return -1 ;
}

Int_t RooMsgService::activeStream(const TObject* self, MsgTopic topic, MsgLevel level) 
{
  // Find appropriate logging stream for message from given object with given topic and message level
  if (level<_globMinLevel) return -1 ;
  for (UInt_t i=0 ; i<_streams.size() ; i++) {
    if (_streams[i].match(level,topic,self)) {
      return i ;
    }
  }
  return -1 ;
}

Bool_t RooMsgService::StreamConfig::match(MsgLevel level, MsgTopic top, const RooAbsArg* obj) 
{
  // Determine if message from given object at given level on given topic is logged
  if (!active) return kFALSE ;
  if (level<minLevel) return kFALSE ;
  if (!(topic&top)) return kFALSE ;

  if (universal) return kTRUE ;

  if (objectName.size()>0 && objectName != obj->GetName()) return kFALSE ;
  if (className.size()>0 && className != obj->IsA()->GetName()) return kFALSE ;
  if (baseClassName.size()>0 && !obj->IsA()->InheritsFrom(baseClassName.c_str())) return kFALSE ;
  if (tagName.size()>0 && !obj->getAttribute(tagName.c_str())) return kFALSE ;
  
  return kTRUE ;
}

Bool_t RooMsgService::StreamConfig::match(MsgLevel level, MsgTopic top, const TObject* obj) 
{
  // Determine if message from given object at given level on given topic is logged
  if (!active) return kFALSE ;
  if (level<minLevel) return kFALSE ;
  if (!(topic&top)) return kFALSE ;

  if (universal) return kTRUE ;
  
  if (objectName.size()>0 && objectName != obj->GetName()) return kFALSE ;
  if (className.size()>0 && className != obj->IsA()->GetName()) return kFALSE ;
  if (baseClassName.size()>0 && !obj->IsA()->InheritsFrom(baseClassName.c_str())) return kFALSE ;
  
  return kTRUE ;
}


ostream& RooMsgService::log(const RooAbsArg* self, MsgLevel level, MsgTopic topic, Bool_t skipPrefix) 
{
  if (level>=ERROR) {
    _errorCount++ ;
  }

  // Return C++ ostream associated with given message configuration
  Int_t as = activeStream(self,topic,level) ;
  if (as==-1) {
    return *_devnull ;
  }

  // Flush any previous messages
  (*_streams[as].os).flush() ;
    
  if (_streams[as].prefix && !skipPrefix) {
    if (_showPid) {
      (*_streams[as].os) << "pid" << getpid() << " " ;
    }
    (*_streams[as].os) << "[#" << as << "] " << _levelNames[level] << ":" << _topicNames[topic]  << " -- " ;
  }
  return (*_streams[as].os) ;
}


ostream& RooMsgService::log(const TObject* self, MsgLevel level, MsgTopic topic, Bool_t skipPrefix) 
{
  if (level>=ERROR) {
    _errorCount++ ;
  }

  // Return C++ ostream associated with given message configuration
  Int_t as = activeStream(self,topic,level) ;
  if (as==-1) {
    return *_devnull ;
  }

  // Flush any previous messages
  (*_streams[as].os).flush() ;
    
  if (_streams[as].prefix && !skipPrefix) {
    if (_showPid) {
      (*_streams[as].os) << "pid" << getpid() << " " ;
    }
    (*_streams[as].os) << "[#" << as << "] " << _levelNames[level] << ":" << _topicNames[topic]  << " -- " ;
  }
  return (*_streams[as].os) ;
}


void RooMsgService::Print(Option_t *options) const 
{
  // Print configuration of message service. If "v" option is given also
  // inactive streams are listed


  Bool_t activeOnly = kTRUE ;
  if (TString(options).Contains("V") || TString(options).Contains("v")) {
    activeOnly = kFALSE ;
  }

  cout << (activeOnly?"Active Message streams":"All Message streams") << endl ;
  for (UInt_t i=0 ; i<_streams.size() ; i++) {

    // Skip passive streams in active only mode
    if (activeOnly && !_streams[i].active) {
      continue ;
    }

    
    map<int,string>::const_iterator is = _levelNames.find(_streams[i].minLevel) ;
    cout << "[" << i << "] MinLevel = " << is->second ;

    cout << " Topic = " ;
    if (_streams[i].topic != 0xFFFFF) {      
      map<int,string>::const_iterator iter = _topicNames.begin() ;
      while(iter!=_topicNames.end()) {
	if (iter->first & _streams[i].topic) {
	  cout << iter->second << " " ;
	}
	++iter ;
      }
    } else {
      cout << " Any " ;
    }
    

    if (_streams[i].objectName.size()>0) {
      cout << " ObjectName = " << _streams[i].objectName ;
    }
    if (_streams[i].className.size()>0) {
      cout << " ClassName = " << _streams[i].className ;
    }
    if (_streams[i].baseClassName.size()>0) {
      cout << " BaseClassName = " << _streams[i].baseClassName ;
    }
    if (_streams[i].tagName.size()>0) {
      cout << " TagLabel = " << _streams[i].tagName ;
    }
    
    // Postfix status when printing all
    if (!activeOnly && !_streams[i].active) {
      cout << " (NOT ACTIVE)"  ;
    }
    
    cout << endl ; 
  }
  
}
