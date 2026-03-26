/*******************************************************************************
   Copyright (C) 2011-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*******************************************************************************/
// File open option define
const SDB_FILE_CREATEONLY =    0x00000001 ;
const SDB_FILE_REPLACE =       0x00000002 ;
const SDB_FILE_CREATE =        SDB_FILE_CREATEONLY | SDB_FILE_REPLACE ;
const SDB_FILE_SHAREREAD =     0x00000010 ;
const SDB_FILE_SHAREWROTE =    SDB_FILE_SHAREREAD | 0x00000020 ;
const SDB_FILE_SHAREWRITE =    SDB_FILE_SHAREREAD | 0x00000020 ;
const SDB_FILE_READONLY =      0x00000004 | SDB_FILE_SHAREREAD ;
const SDB_FILE_WRITEONLY =     0x00000008 ;
const SDB_FILE_READWRITE =     0x00000004 | SDB_FILE_WRITEONLY ;


const SDB_INIFILE_NOTCASE          = 0x00000001 ;

//Support annotation symbols ( ; )
const SDB_INIFILE_SEMICOLON        = 0x00000002 ;

//Support annotation symbols ( # )
const SDB_INIFILE_HASHMARK         = 0x00000004 ;

//Support escape character   ( '\\' )
const SDB_INIFILE_ESCAPE           = 0x00000008 ;

//Support Double quotation mark ( " )
const SDB_INIFILE_DOUBLE_QUOMARK   = 0x00000010 ;

//Support Single quotation mark ( ' )
const SDB_INIFILE_SINGLE_QUOMARK   = 0x00000020 ;

//Support Colon ( = )
const SDB_INIFILE_EQUALSIGN        = 0x00000040 ;

//Support Colon ( : )
const SDB_INIFILE_COLON            = 0x00000080 ;

const SDB_INIFILE_UNICODE          = 0x00010000 ;

//The same section name and key are not allowed
const SDB_INIFILE_STRICTMODE       = 0x00020000 ;

const SDB_INIFILE_FLAGS_DEFAULT    = SDB_INIFILE_SEMICOLON | SDB_INIFILE_EQUALSIGN | SDB_INIFILE_STRICTMODE ;
const SDB_INIFILE_FLAGS_MYSQL      = SDB_INIFILE_HASHMARK | SDB_INIFILE_EQUALSIGN | SDB_INIFILE_STRICTMODE ;
const SDB_INIFILE_FLAGS_POSTGRESQL = SDB_INIFILE_ESCAPE | SDB_INIFILE_HASHMARK | SDB_INIFILE_EQUALSIGN | SDB_INIFILE_SINGLE_QUOMARK | SDB_INIFILE_STRICTMODE ;


var SDB_PRINT_JSON_FORMAT        = true ;

function jsonFormat(pretty) {
   if (pretty == undefined){
      pretty = true;
   }
   SDB_PRINT_JSON_FORMAT = pretty;
}

// BSONObj
BSONObj.prototype.toObj = function() {
   return JSON.parse( this.toJson() ) ;
}

BSONObj.prototype.toString = function() {
   if ( typeof(SDB_PRINT_JSON_FORMAT) == "undefined" ||
        SDB_PRINT_JSON_FORMAT )
   {
      try
      {
         var obj = this.toObj();
         var str = JSON.stringify ( obj, undefined, 2 ) ;
         return str ;
      }
      catch ( e )
      {
         return this.toJson() ;
      }
   }
   else
   {
      return this.toJson() ;
   }
}
// end BSONObj

// BSONArray
BSONArray.prototype.toArray = function() {
   if ( this._arr )
      return this._arr;

   var a = [];
   while ( true ) {
      var bs = this.next();
      if ( ! bs ) break ;
      var json = bs.toJson () ;
      try
      {
         var stf = JSON.stringify(JSON.parse(json), undefined, 2) ;
         a.push ( stf ) ;
      }
      catch ( e )
      {
         a.push ( json ) ;
      }
   }
   this._arr = a ;
   return this._arr ;
}

BSONArray.prototype.arrayAccess = function( idx ) {
   return this.toArray()[idx] ;
}

BSONArray.prototype.toString = function() {
   //return this.toArray().join('\n') ;
   var array = this ;
   var record = undefined ;
   var returnRecordNum = 0 ;
   while ( ( record = array.next() ) != undefined )
   {
      returnRecordNum++ ;
      try
      {
         println ( record ) ;
      }
      catch ( e )
      {
         var json = record.toJson () ;
         println ( json ) ;
      }
   }
   println("Return "+returnRecordNum+" row(s).") ;
   return "" ;
}

BSONArray.prototype._formatStr = function() {

   var bsonObj = this.toArray() ;
   var objArr = new Array() ;
   var eleArr = new Array() ;
   var maxSizeArr = new Array() ;
   var outStr = "" ;
   var colNameArr = new Array() ;
   var objNum ;

   for ( var i in bsonObj )
   {
      objArr.push( JSON.parse( bsonObj[i] ) ) ;
   }

   var objNum = objArr.length ;

   if ( objNum > 0 )
   {
      // get row name and init maxSizeArr
      for( var index in objArr )
      {
         for ( var eleKey in objArr[ index ] )
         {
            if ( -1 == colNameArr.indexOf( eleKey ) )
            {
               colNameArr.push( eleKey ) ;
               maxSizeArr[ eleKey ] = eleKey.length ;
            }
         }
      }

      for ( var index in objArr )
      {
         var localArr = new Array() ;
         for( var ele in objArr[ index ] )
         {
            var localEle = objArr[ index ][ ele ].toString() ;
            localArr[ ele ] = localEle ;
            if ( maxSizeArr[ ele ] < localEle.length )
            {
               maxSizeArr[ ele ] = localEle.length ;
            }
         }
         eleArr.push( localArr ) ;
      }

      for( var index in maxSizeArr )
      {
         maxSizeArr[ index ] = maxSizeArr[ index ] + 1 ;
      }
      for ( var index in colNameArr )
      {
         var localRowName = colNameArr[ index ] ;
         outStr += " " + localRowName ;
         for ( var k = 0; k < maxSizeArr[ localRowName ] - localRowName.length ;
               k++ )
         {
            outStr += " " ;
         }
      }
      outStr += "\n" ;

      for ( var index in eleArr )
      {
         var arr = eleArr[ index ] ;
         for ( var ele in colNameArr )
         {
            var localRowName = colNameArr[ ele ] ;
            var localEle = arr[ localRowName ] ;
            if ( undefined == localEle )
            {
               localEle = "" ;
            }
            outStr += " " + localEle ;
            for ( var k = 0; k < maxSizeArr[ localRowName ] - localEle.length;
                  k++ )
            {
               outStr += " " ;
            }
         }
         outStr += "\n" ;
      }
   }
   return outStr ;
}
// end BSONArray

// Oma member function
Oma.prototype.getOmaInstallFile = function() {
   return this._runCommand( "oma get oma install file" ).toObj().installFile ;
}

Oma.prototype.getOmaInstallInfo = function() {
   return this._runCommand( "oma get oma install info" ) ;
}

Oma.prototype.getOmaConfigFile = function() {
   return this._runCommand( "oma get oma config file" ).toObj().confFile ;
}

Oma.prototype.getOmaConfigs = function( confFile ) {
   var recvObj ;

   // run command
   if ( undefined != confFile )
   {
      recvObj = this._runCommand( "oma get oma configs", {},
                                  { "confFile": confFile } ) ;
   }
   else
   {
      recvObj = this._runCommand( "oma get oma configs" ) ;
   }
   return recvObj ;
}

Oma.prototype.getIniConfigs = function( confFile, options ) {
   var recvObj ;
   var newOptions = options ;

   if ( undefined == newOptions )
   {
      newOptions = {} ;
   }

   newOptions["confFile"] = confFile ;

   recvObj = this._runCommand( "oma get ini configs", newOptions ) ;

   return recvObj ;
}

Oma.prototype.setIniConfigs = function( configs, confFile, options ) {
   var newOptions = options ;

   if ( undefined == newOptions )
   {
      newOptions = {} ;
   }

   newOptions["confFile"] = confFile ;

   this._runCommand( "oma set ini configs", newOptions, configs ) ;
}

Oma.prototype.setOmaConfigs = function( configsObj, confFile, isReload ) {

   // check argument
   if ( undefined == configsObj )
   {
      setLastErrMsg( "obj must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   // run command
   if ( undefined != confFile && "" != confFile )
   {
      this._runCommand( "oma set oma configs", {}, { "confFile": confFile },
                        { "configsObj": configsObj } ) ;
   }
   else
   {
      this._runCommand( "oma set oma configs", {}, {},
                        { "configsObj": configsObj } ) ;
   }

   if ( true == isReload )
   {
      this.reloadConfigs() ;
   }
}

Oma.prototype.getAOmaSvcName = function( hostname, confFile ) {
   // check argument
   if ( undefined == hostname )
   {
      setLastErrMsg( "hostname must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }
   var recvObj ;

   // run command
   if ( undefined != confFile )
   {
      recvObj = this._runCommand( "oma get a oma svc name", {},
                                  { "confFile": confFile,
                                    "hostname": hostname } ) ;
   }
   else
   {
      recvObj = this._runCommand( "oma get a oma svc name", {},
                                  { "hostname": hostname } ) ;
   }
   return recvObj.toObj().svcName ;
}

Oma.prototype.addAOmaSvcName = function( hostname, svcname,
                                         isReplace, confFile ) {
   // check argument
   if ( undefined == hostname  )
   {
      setLastErrMsg( "hostname must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }
   else if ( undefined == svcname )
   {
      setLastErrMsg( "svcname must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   if ( undefined == isReplace )
   {
      isReplace = true ;
   }

   // run command
   if ( undefined != confFile )
   {
      this._runCommand( "oma add a oma svc name", { "isReplace": isReplace },
                        { "confFile": confFile },
                        { "hostname": hostname, "svcname": svcname } ) ;
   }
   else
   {
      this._runCommand( "oma add a oma svc name", { "isReplace": isReplace },
                        {}, { "hostname": hostname, "svcname": svcname } ) ;
   }
}

Oma.prototype.delAOmaSvcName = function( hostname, confFile ) {

   // check argument
   if ( undefined == hostname  )
   {
      setLastErrMsg( "hostname must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   // run command
   if ( undefined != confFile )
   {
      this._runCommand( "oma del a oma svc name", {},
                        { "hostname": hostname, "confFile": confFile } ) ;
   }
   else
   {
      this._runCommand( "oma del a oma svc name", {},
                        { "hostname": hostname } ) ;
   }
}

Oma.prototype.listNodes = function( optionObj, filterObj ) {
   var displayMode ;

   // check argument
   if ( undefined != optionObj )
   {
      if ( "object" != typeof( optionObj ) )
      {
         setLastErrMsg( "optionObj must be object" ) ;
         throw SDB_INVALIDARG ;
      }

      if ( undefined != optionObj.displayMode )
      {
         displayMode = optionObj.displayMode ;
         delete optionObj.displayMode ;
      }
   }
   else
   {
      optionObj = {} ;
   }

   // run command
   var recvObj = this._runCommand( "oma list nodes", optionObj ) ;
   var retArray ;

   // filter
   if ( undefined != filterObj )
   {
      var filter = new _Filter( filterObj ) ;
      retArray = filter.match( recvObj.toObj() ) ;
   }
   else
   {
      var filter = new _Filter( {} ) ;
      retArray = filter.match( recvObj.toObj() ) ;
   }

   // set display format
   if ( "text" == displayMode )
   {
      retArray = retArray._formatStr() ;
   }
   return retArray ;
}

Oma.prototype.getNodeConfigs = function( svcname ) {
   // check svcname
   if ( undefined == svcname )
   {
      setLastErrMsg( "svcname must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   // run command
   return this._runCommand( "oma get node configs",
                            {}, { "svcname": svcname } ) ;
}

Oma.prototype.setNodeConfigs = function( svcname, configsObj ) {
   // check argument
   if ( undefined == svcname )
   {
      setLastErrMsg( "svcname must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }
   if ( undefined == configsObj )
   {
      setLastErrMsg( "configsObj must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   // run command
   this._runCommand( "oma set node configs", {},
                     { "svcname": svcname }, { "configsObj": configsObj } ) ;
}

Oma.prototype.updateNodeConfigs = function( svcname, configsObj ) {
   // check argument
   if ( undefined == svcname )
   {
      setLastErrMsg( "svcname must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }
   if ( undefined == configsObj )
   {
      setLastErrMsg( "configsObj must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   // run command
   this._runCommand( "oma update node configs", {},
                     { "svcname": svcname }, { "configsObj": configsObj } ) ;
}

Oma.prototype.startAllNodes = function( businessName ) {
   var localNodes ;
   if ( 'string' == typeof( businessName ) )
   {
      localNodes = this.listNodes( { 'type': 'db',
                                     'expand': true,
                                     'mode': 'local' },
                                   { 'businessname': businessName } ).toArray() ;
   }
   else
   {
      localNodes = this.listNodes( { 'type': 'db', 'expand': true,
                                     'mode': 'local' } ).toArray() ;
   }

   var svcname = [] ;

   for( var index in localNodes )
   {
      obj = JSON.parse( localNodes[ index ] ) ;
      svcname.push( obj['svcname'] ) ;
   }

   var total = svcname.length
   var failed = 0 ;
   var failedList = [] ;
   var failedInfo = {} ;

   var result = this._nodesOperation( 'start', svcname ) ;

   if ( result['errno'] && Array.isArray( result['ErrNodes'] ) )
   {
      for( var i in result['ErrNodes'] )
      {
         failedList.push( result['ErrNodes'][i]['svcname'] ) ;

         failedInfo[result['ErrNodes'][i]['svcname']] = result['ErrNodes'][i] ;
      }

      failed = failedList.length ;
   }

   for( var i in svcname )
   {
      print( "Start sequoiadb(" + svcname[i] + ") " ) ;

      if ( failedList.indexOf( svcname[i] ) < 0 )
      {
         println( "success" ) ;
      }
      else
      {
         println( "failed, errno: " + failedInfo[svcname[i]]["errno"] +
                  ", description: " + failedInfo[svcname[i]]["description"] +
                  ", detail: " + failedInfo[svcname[i]]["detail"] ) ;
      }
   }

   println( "Total: " + total + "; Success: " + ( total - failed ) + "; Failed: " + failed ) ;

   if ( result['errno'] )
   {
      setLastErrObj( result ) ;
      setLastErrMsg( result['description'] ) ;
      throw result['errno'] ;
   }
}

Oma.prototype.stopAllNodes = function( businessName ) {
   var localNodes ;
   if ( 'string' == typeof( businessName ) )
   {
      localNodes = this.listNodes( { 'type': 'db',
                                     'expand': true,
                                     'mode': "run" },
                                   { 'businessname': businessName } ).toArray() ;
   }
   else
   {
      localNodes = this.listNodes( { 'type': 'db', 'expand': true,
                                     'mode': 'run' } ).toArray() ;
   }

   var svcname = [] ;

   for( var index in localNodes )
   {
      obj = JSON.parse( localNodes[ index ] ) ;
      svcname.push( obj['svcname'] ) ;
   }

   var total = svcname.length
   var failed = 0 ;
   var failedList = [] ;

   var result = this._nodesOperation( 'stop', svcname ) ;

   if ( result['errno'] && Array.isArray( result['ErrNodes'] ) )
   {
      for( var i in result['ErrNodes'] )
      {
         failedList.push( result['ErrNodes'][i]['svcname'] ) ;
      }

      failed = failedList.length ;
   }

   for( var i in svcname )
   {
      print( "Stop sequoiadb(" + svcname[i] + ") " ) ;

      if ( failedList.indexOf( svcname[i] ) < 0 )
      {
         println( "success" ) ;
      }
      else
      {
         println( "failed" ) ;
      }
   }

   println( "Total: " + total + "; Success: " + ( total - failed ) + "; Failed: " + failed ) ;

   if ( result['errno'] )
   {
      setLastErrObj( result ) ;
      setLastErrMsg( result['description'] ) ;
      throw result['errno'] ;
   }
}

Oma.prototype._checkSvcname = function( svcname ) {
   var type = typeof( svcname ) ;

   if ( 'undefined' == type )
   {
      setLastErrMsg( "svcname must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }
   else if ( 'number' == type && Math.round( svcname ) === svcname )
   {
      if ( svcname <= 0 || svcname > 65535 )
      {
         setLastErrMsg( "The port range of svcname is (0, 65535]" ) ;
         throw SDB_INVALIDARG ;
      }
   }
   else if ( 'string' == type )
   {
   }
   else
   {
      setLastErrMsg( "svcname must be string or integer or array" ) ;
      throw SDB_INVALIDARG ;
   }
}

Oma.prototype._nodesOperation = function( cmd, svcname ) {
   var command = "oma " + cmd + " nodes" ;
   var options = {} ;

   if ( [ 'start', 'stop' ].indexOf( cmd ) < 0 )
   {
      setLastErrMsg( "Invalid command" ) ;
      throw SDB_INVALIDARG ;
   }

   if ( Array.isArray( svcname ) )
   {
      for( var i in svcname )
      {
         this._checkSvcname( svcname[i] ) ;
      }

      options['svcname'] = svcname ;
   }
   else
   {
      this._checkSvcname( svcname ) ;
      options['svcname'] = [ svcname ] ;
   }

   return this._runCommand( command, options ).toObj() ;
}

Oma.prototype.startNodes = function( svcname ) {
   var result = this._nodesOperation( 'start', svcname ) ;

   if ( result['errno'] )
   {
      setLastErrObj( result ) ;
      setLastErrMsg( result['description'] ) ;
      throw result['errno'] ;
   }
}

Oma.prototype.stopNodes = function( svcname ) {
   var result = this._nodesOperation( 'stop', svcname ) ;

   if ( result['errno'] )
   {
      setLastErrObj( result ) ;
      setLastErrMsg( result['description'] ) ;
      throw result['errno'] ;
   }
}

Oma.prototype.reloadConfigs = function()
{
   this._runCommand( "reload config" ) ;
}
// end Oma

// Remote member function
Remote.prototype.getSystem = function() {
   var system = System.getObj() ;
   system._remote = this ;
   return system ;
}

Remote.prototype.getFile = function( filename, permission, openMode ) {
   var file = File._getFileObj() ;
   var option = {} ;
   file._remote = this ;


   if ( 1 <= arguments.length )
   {
      if ( "string" != typeof( filename ) )
      {
         setLastErrMsg( "filename must be string" ) ;
         throw SDB_INVALIDARG ;
      }

      if( undefined != permission )
      {
         option.permission = permission ;
         if( undefined != openMode )
         {
            option.mode = openMode ;
         }
      }

      var retObj = this._runCommand( "file open", option, {},
                                     { "Filename": filename } ) ;
      file._FID = retObj.toObj().FID ;
      file._filename = filename ;
   }
   return file ;
}

Remote.prototype.getIniFile = function( filename, flags ) {
   var file    = this.getFile( filename, 0, SDB_FILE_READONLY ) ;
   var length  = file.getSize( filename ) ;
   var content = file.read( length ) ;
   var ini ;

   file.close() ;

   if ( undefined == flags )
   {
      flags = 0 ;
   }

   try
   {
      ini = new IniFile( filename, flags, content ) ;
   }
   catch( e )
   {
      if ( typeof( e ) == 'number' )
      {
         var msg = getLastErrMsg() ;
         var result = {
            'errno': e,
            'description': msg,
            'detail': msg
         } ;
         setLastErrObj( result ) ;
      }

      throw e ;
   }

   ini._remote = this ;

   return ini ;
}

Remote.prototype.getCmd = function() {
   var cmd = new Cmd() ;
   cmd._remote = this ;
   cmd._retCode = SDB_OK ;
   cmd._strOut = '' ;
   cmd._command = '' ;
   return cmd ;
}

Remote.prototype._runCommand = function( command, optionObj,
                                         matchObj, valueObj ) {
   var bsonObj ;
   var retObj ;
   var option = {} ;
   var match = {} ;
   var value = {} ;
   if ( 4 < arguments.length )
   {
      setLastErrMsg( "Too much arguments" ) ;
      throw SDB_INVALIDARG ;
   }
   if( undefined != optionObj )
   {
      option = optionObj ;
   }
   if( undefined != matchObj )
   {
      match = matchObj ;
   }
   if( undefined != valueObj )
   {
      value = valueObj ;
   }

   try
   {
      bsonObj = this.__runCommand( command, option, match, value ) ;
   }
   catch( e )
   {
      var errValue = e.message || e;
      if( SDB_INVALIDARG == errValue || SDB_OUT_OF_BOUND == errValue )
      {
         var errMsg = getLastErrMsg() ;
         var errObj = getLastErrObj().toObj() ;
         var extraErrMsg = ": the cause of this error may be that the server version "
                           + "is not consistent with the client version" ;
         errObj.detail = errObj.detail + extraErrMsg ;
         setLastErrMsg( errMsg + extraErrMsg  ) ;
         setLastErrObj( errObj ) ;
      }
      throw e ;
   }

   return bsonObj ;
}
// end Remote

// _Filter member function
_Filter.prototype.match = function( BSONArrObj ) {

   if ( BSONArrObj instanceof Object )
   {
      return this._match( BSONArrObj ) ;
   }
   else
   {
      setLastErrMsg( "Argument must be objArray" ) ;
      throw SDB_INVALIDARG ;
   }
}
// end _Filter

// System static function
System.listProcess = function( optionObj, filterObj )
{
   var result ;
   var recvObj ;
   var displayMode = "obj" ;

   // check argument
   if ( undefined != optionObj )
   {
      if ( undefined != optionObj.displayMode )
      {
         displayMode = optionObj.displayMode ;
         delete optionObj.displayMode ;
      }
      recvObj = System._listProcess( optionObj ) ;
   }
   else
   {
      recvObj = System._listProcess() ;
   }

   // filter result
   if ( undefined != filterObj )
   {
      var filter = new _Filter( filterObj ) ;
      result = filter.match( recvObj.toObj() ) ;
   }
   else
   {
      var filter = new _Filter( {} ) ;
      result = filter.match( recvObj.toObj() ) ;
   }

   // set format
   if ( "text" == displayMode )
   {
      return result._formatStr() ;
   }
   else
   {
      return result ;
   }
}

System.isProcExist = function( optionObj ) {
   var retArray ;
   var isExist ;

   // check argument
   if ( undefined != optionObj )
   {
      if ( undefined == optionObj.value )
      {
         setLastErrMsg( "value must be config" ) ;
         throw SDB_OUT_OF_BOUND ;
      }
      else
      {
         // get specific process
         if ( optionObj.type == "name" )
         {
             retArray = System.listProcess( { "detail": true },
                                            { "cmd": optionObj.value } ) ;
         }
         else
         {
            retArray = System.listProcess( { "detail": true },
                                           { "pid": optionObj.value } ) ;
         }
      }
   }
   else
   {
      setLastErrMsg( "optionObj must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   if ( 0 != retArray.size() )
   {
      isExist = true ;
   }
   else
   {
      isExist = false ;
   }
   return isExist ;
}

System.listLoginUsers = function( optionObj, filterObj ) {
   var objArray ;
   var retArray ;
   var displayMode = "obj" ;

   // check argument
   if ( undefined != optionObj )
   {
      if ( undefined != optionObj.displayMode )
      {
         displayMode = optionObj.displayMode ;
         delete optionObj.displayMode ;
      }
      objArray = System._listLoginUsers( optionObj ) ;
   }
   else
   {
      objArray = System._listLoginUsers() ;
   }

   // filter
   if ( undefined != filterObj )
   {
      var filter = new _Filter( filterObj ) ;
      retArray = filter.match( objArray.toObj() ) ;
   }
   else
   {
      var filter = new _Filter( {} ) ;
      retArray = filter.match( objArray.toObj() ) ;
   }

   // set display format
   if ( "text" == displayMode )
   {
      retArray = retArray._formatStr() ;
   }
   return retArray ;
}

System.listAllUsers = function( optionObj, filterObj ) {
   var objArray ;
   var retArray ;
   var displayMode = "obj" ;

   // check argument
   if ( undefined != optionObj )
   {
      if ( undefined != optionObj.displayMode )
      {
         displayMode = optionObj.displayMode ;
         delete optionObj.displayMode ;
      }
      objArray = System._listAllUsers( optionObj ) ;
   }
   else
   {
      objArray = System._listAllUsers() ;
   }

   // filter
   if ( undefined != filterObj )
   {
      var filter = new _Filter( filterObj ) ;
      retArray = filter.match( objArray.toObj() ) ;
   }
   else
   {
      var filter = new _Filter( {} ) ;
      retArray = filter.match( objArray.toObj() ) ;
   }

   // set display format
   if ( "text" == displayMode )
   {
      retArray = retArray._formatStr() ;
   }

   return retArray ;
}

System.listGroups = function( optionObj, filterObj ) {
   var objArray ;
   var retArray ;
   var displayMode = "obj" ;

   // check argument
   if ( undefined != optionObj )
   {
      if ( undefined != optionObj.displayMode )
      {
         displayMode = optionObj.displayMode ;
         delete optionObj.displayMode ;
      }
      objArray = System._listGroups( optionObj ) ;
   }
   else
   {
      objArray = System._listGroups() ;
   }

   // filter
   if ( undefined != filterObj )
   {
      var filter = new _Filter( filterObj ) ;
      retArray = filter.match( objArray.toObj() ) ;
   }
   else
   {
      var filter = new _Filter( {} ) ;
      retArray = filter.match( objArray.toObj() ) ;
   }

   // set display format
   if ( "text" == displayMode )
   {
      retArray = retArray._formatStr() ;
   }
   return retArray ;
}

System.isUserExist = function( userName ) {
   var isExist = false ;
   var retArray ;

   // check argument
   if ( undefined == userName )
   {
      setLastErrMsg( "userName must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   // get specific result
   retArray = System.listAllUsers( { "detail": true }, { "user": userName } ) ;

   if ( 0 != retArray.size() )
   {
      isExist = true ;
   }
   return isExist ;
}

System.isGroupExist = function( groupName ) {
   var isExist = false ;
   var retArray ;

   // check argument
   if ( undefined == groupName )
   {
      setLastErrMsg( "userName must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   // get specific result
   retArray = System.listGroups( { "detail": true }, { "name": groupName } ) ;

   if ( 0 != retArray.size() )
   {
      isExist = true ;
   }
   return isExist ;
}

// System member function
System.prototype.getInfo = function()
{
   var result ;
   var infoObj ;
   if ( undefined != this._remote )
   {
      var _remoteInfo = this._remote.getInfo() ;
      infoObj = _remoteInfo.toObj() ;
      infoObj.isRemote = true ;
   }
   else
   {
      infoObj = new Object() ;
      infoObj.isRemote = false ;
   }
   result = this._getInfo( infoObj ) ;
   return result ;
}

System.prototype.ping = function( hostname ) {
   var retObj ;

   if ( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "system ping", {},
                                             { "hostname" : hostname } ) ;
   }
   else
   {
      retObj = System.ping( hostname ) ;
   }
   return retObj ;

}

System.prototype.type = function() {
   var retStr ;

   // check argument
   if ( 0 < arguments.length )
   {
      setLastErrMsg( getErr( SDB_INVALIDARG ) ) ;
      throw SDB_INVALIDARG ;
   }

   if ( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "system type" ) ;
      retStr = retObj.toObj().type ;
   }
   else
   {
      retStr = System.type() ;
   }
   return retStr ;
}

System.prototype.getReleaseInfo = function() {
   var retObj ;

   // check argument
   if ( 0 < arguments.length )
   {
      setLastErrMsg( getErr( SDB_INVALIDARG ) ) ;
      throw SDB_INVALIDARG ;
   }

   if ( undefined != this._remote )
   {
      retObj = this._remote._runCommand( "system get release info" ) ;
   }
   else
   {
      retObj = System.getReleaseInfo() ;
   }
   return retObj ;
}

System.prototype.getHostsMap = function() {
   var retObj ;

   if ( 0 < arguments.length )
   {
      setLastErrMsg( "getHostsMap() should have non arguments" ) ;
      throw SDB_INVALIDARG ;
   }

   if ( undefined != this._remote )
   {
      retObj = this._remote._runCommand( "system get hosts map" ) ;
   }
   else
   {
      retObj = System.getHostsMap() ;
   }
   return retObj ;
}

System.prototype.getAHostMap = function( hostname ) {
   var retStr ;

   if ( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "system get a host map", {},
                                             { "hostname": hostname } ) ;
      retStr = retObj.toObj().ip ;
   }
   else
   {
      retStr = System.getAHostMap() ;
   }
   return retStr ;
}

System.prototype.addAHostMap = function( hostname, ip, isReplace ) {
   if ( undefined == isReplace )
   {
      isReplace = true ;
   }
   if ( undefined != this._remote )
   {
      this._remote._runCommand( "system add a host map", {}, {},
                                { "hostname": hostname,
                                  "ip": ip,
                                  "isReplace": isReplace } ) ;
   }
   else
   {
      System.addAHostMap( hostname, ip, isReplace ) ;
   }
}

System.prototype.delAHostMap = function( hostname ) {
   if ( undefined != this._remote )
   {
      this._remote._runCommand( "system delete a host map", {},
                                { "hostname": hostname } ) ;
   }
   else
   {
      System.delAHostMap( hostname ) ;
   }
}

System.prototype.getCpuInfo = function() {
   var retObj ;
   if ( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "system get cpu info" ) ;
   }
   else
   {
      retObj = System.getCpuInfo() ;
   }
   return retObj ;
}

System.prototype.snapshotCpuInfo = function() {
   var retObj ;
   if ( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "system snapshot cpu info" ) ;
   }
   else
   {
      retObj = System.snapshotCpuInfo() ;
   }
   return retObj ;
}

System.prototype.getMemInfo = function() {
   var retObj ;
   if ( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "system get mem info" ) ;
   }
   else
   {
      retObj = System.getMemInfo() ;
   }
   return retObj ;
}

System.prototype.snapshotMemInfo = function() {
   var retObj ;
   if ( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "system get mem info" ) ;
   }
   else
   {
      retObj = System.snapshotMemInfo() ;
   }
   return retObj ;
}

System.prototype.getDiskInfo = function() {
   var retObj ;
   if ( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "system get disk info" ) ;
   }
   else
   {
      retObj = System.getDiskInfo() ;
   }
   return retObj ;
}

System.prototype.snapshotDiskInfo = function() {
   var retObj ;
   if ( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "system get disk info" ) ;
   }
   else
   {
      retObj = System.snapshotDiskInfo() ;
   }
   return retObj ;
}

System.prototype.getNetcardInfo = function() {
   var retObj ;
   if ( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "system get netcard info" ) ;
   }
   else
   {
      retObj = System.getNetcardInfo() ;
   }
   return retObj ;
}

System.prototype.snapshotNetcardInfo = function() {
   var retObj ;
   if ( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "system snapshot netcard info" ) ;
   }
   else
   {
      retObj = System.snapshotNetcardInfo() ;
   }
   return retObj ;
}

System.prototype.getIpTablesInfo = function() {
   var retObj ;
   if ( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "system get ip tables info" ) ;
   }
   else
   {
      retObj = System.getIpTablesInfo() ;
   }
   return retObj ;
}

System.prototype.getHostName = function() {
   var result ;

   // check argument
   if ( 0 < arguments.length )
   {
      setLastErrMsg( getErr( SDB_INVALIDARG ) ) ;
      throw SDB_INVALIDARG ;
   }

   if ( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "system get hostname" ) ;
      result = retObj.toObj().hostname ;
   }
   else
   {
      result = System.getHostName() ;
   }
   return result ;
}

System.prototype.sniffPort = function( port ) {
   var retObj ;

   // check argument
   if ( undefined == port )
   {
      setLastErrMsg( "not specified the port to sniff" ) ;
      throw SDB_INVALIDARG ;
   }

   if ( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "system sniff port", {},
                                             { "port": port }) ;
   }
   else
   {
      retObj = System.sniffPort( port ) ;
   }
   return retObj ;
}

System.prototype.listProcess = function( optionObj, filterObj ) {
   var retObj ;
   var result ;

   if ( undefined != this._remote )
   {
      var displayMode = "obj" ;

      // check argument
      if ( undefined != optionObj )
      {
         if ( "object" != typeof( optionObj ) )
         {
            setLastErrMsg( "optionObj must be object" ) ;
            throw SDB_INVALIDARG ;
         }
         if ( undefined != optionObj.displayMode  )
         {
            displayMode = optionObj.displayMode ;
            delete optionObj.displayMode ;
         }
      }

      if ( undefined != optionObj )
      {
         result = this._remote._runCommand( "system list process", optionObj ) ;
      }
      else
      {
         result = this._remote._runCommand( "system list process" ) ;
      }

      if ( undefined != filterObj )
      {
         var filter = new _Filter( filterObj ) ;
         retObj = filter.match( result.toObj() ) ;
      }
      else
      {
         var filter = new _Filter( {} ) ;
         retObj = filter.match( result.toObj() ) ;
      }

      if ( "text" == displayMode )
      {
         return retObj._formatStr() ;
      }
      else
      {
         return retObj ;
      }
   }
   else
   {
      retObj = System.listProcess( optionObj, filterObj ) ;
      return retObj ;
   }
}

System.prototype.isProcExist = function( optionObj ) {

   var retArray ;
   var isExist = false ;

   // check argument
   if ( undefined != optionObj )
   {
      if ( undefined == optionObj.value )
      {
         setLastErrMsg( "value must be config" ) ;
         throw SDB_OUT_OF_BOUND ;
      }
      else
      {
         if ( "name" == optionObj.type )
         {
             retArray = this.listProcess( { "detail": true },
                                         { "cmd": optionObj.value } ) ;
           }
         else
         {
            retArray = this.listProcess( { "detail": true },
                                         { "pid": optionObj.value } ) ;
         }
      }

     }
   else
   {
      setLastErrMsg( "optionObj must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   if ( 0 != retArray.size() )
   {
      isExist = true ;
   }
   return isExist ;
}

System.prototype.addUser = function( userObj ) {
   if ( undefined != this._remote )
   {
      this._remote._runCommand( "system add user", {}, {}, userObj ) ;
   }
   else
   {
      System.addUser( userObj ) ;
   }
}

System.prototype.setUserConfigs = function( optionObj ) {
   if ( undefined != this._remote )
   {
      this._remote._runCommand( "system set user configs", {}, {}, optionObj ) ;
   }
   else
   {
      System.setUserConfigs( optionObj ) ;
   }
}

System.prototype.delUser = function( optionObj ) {
   if ( undefined != this._remote )
   {
      this._remote._runCommand( "system del user", {}, optionObj ) ;
   }
   else
   {
      System.delUser( optionObj ) ;
   }
}

System.prototype.addGroup = function( optionObj ) {
   if ( undefined != this._remote )
   {
      this._remote._runCommand( "system add group", {}, {}, optionObj ) ;
   }
   else
   {
      System.addGroup( optionObj ) ;
   }
}

System.prototype.delGroup = function( name ) {
   if ( undefined != this._remote )
   {
      this._remote._runCommand( "system del group", {}, { "name" :name } ) ;
   }
   else
   {
      System.delGroup( name ) ;
   }
}

System.prototype.listLoginUsers = function( optionObj, filterObj ) {
   var retArray ;

   if ( undefined != this._remote )
   {
      var objArray ;
      var displayMode = "obj" ;

      // check argument
      if ( undefined != optionObj )
      {
         if ( undefined != optionObj.displayMode )
         {
            displayMode = optionObj.displayMode ;
            delete optionObj.displayMode ;
         }
         objArray = this._remote._runCommand( "system list login users",
                                              optionObj ) ;
      }
      else
      {
         objArray = this._remote._runCommand( "system list login users" ) ;
      }

      // filter
      if ( undefined != filterObj )
      {
         var filter = new _Filter( filterObj ) ;
         retArray = filter.match( objArray.toObj() ) ;
      }
      else
      {
         var filter = new _Filter( {} ) ;
         retArray = filter.match( objArray.toObj() ) ;
      }

      // set format
      if ( "text" == displayMode )
      {
         retArray = retArray._formatStr() ;
      }
   }
   else
   {
      retArray = System.listLoginUsers( optionObj, filterObj ) ;
   }
   return retArray ;
}

System.prototype.listAllUsers = function( optionObj, filterObj ) {
   var retArray ;

   if ( undefined != this._remote )
   {
      var objArray ;
      var displayMode = "obj" ;

      // check argument
      if ( undefined != optionObj )
      {
         if ( undefined != optionObj.displayMode )
         {
            displayMode = optionObj.displayMode ;
            delete optionObj.displayMode ;
         }
         objArray = this._remote._runCommand( "system list all users",
                                              optionObj ) ;
      }
      else
      {
         objArray = this._remote._runCommand( "system list all users" ) ;
      }

      // filter
      if ( undefined != filterObj )
      {
         var filter = new _Filter( filterObj ) ;
         retArray = filter.match( objArray.toObj() ) ;
      }
      else
      {
         var filter = new _Filter( {} ) ;
         retArray = filter.match( objArray.toObj() ) ;
      }

      // set display format
      if ( "text" == displayMode )
      {
         retArray = retArray._formatStr() ;
      }
   }
   else
   {
      retArray = System.listAllUsers( optionObj, filterObj ) ;
   }
   return retArray ;
}

System.prototype.listGroups = function( optionObj, filterObj ) {
   var retArray ;

   if ( undefined != this._remote )
   {
      var objArray ;
      var displayMode = "obj" ;

      // check argument
      if ( undefined != optionObj )
      {
         if ( undefined != optionObj.displayMode )
         {
            displayMode = optionObj.displayMode ;
            delete optionObj.displayMode ;
         }
         objArray = this._remote._runCommand( "system list all groups",
                                              optionObj ) ;
      }
      else
      {
         objArray = this._remote._runCommand( "system list all groups" ) ;
      }

      // filter
      if ( undefined != filterObj )
      {
         var filter = new _Filter( filterObj ) ;
         retArray = filter.match( objArray.toObj() ) ;
      }
      else
      {
         var filter = new _Filter( {} ) ;
         retArray = filter.match( objArray.toObj() ) ;
      }

      // set display format
      if ( "text" == displayMode )
      {
         retArray = retArray._formatStr() ;
      }
   }
   else
   {
      retArray = System.listGroups( optionObj, filterObj ) ;
   }

   return retArray ;
}

System.prototype.getCurrentUser = function() {
   var retObj ;

   if ( undefined != this._remote )
   {
      retObj = this._remote._runCommand( "system get current user" ) ;
   }
   else
   {
      retObj = System.getCurrentUser() ;
   }
   return retObj ;
}

System.prototype.isUserExist = function( userName ) {
   var isExist = false ;
   var retArray ;

   // check argument
   if ( undefined == userName )
   {
      setLastErrMsg( "userName must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   if ( undefined != this._remote )
   {
      retArray = this.listAllUsers( { "detail": true }, { "user": userName } ) ;
   }
   else
   {
      retArray = System.listAllUsers( { "detail": true }, { "user": userName } ) ;
   }

   // check result
   if ( 0 != retArray.size() )
   {
      isExist = true ;
   }
   return isExist ;
}

System.prototype.isGroupExist = function( groupName ) {
   var isExist = false ;
   var retArray ;

   // check argument
   if ( undefined == groupName )
   {
      setLastErrMsg( "groupName must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   if ( undefined != this._remote )
   {
      retArray = this.listGroups( { "detail": true }, { "name": groupName } ) ;
   }
   else
   {
      retArray = System.listGroups( { "detail": true }, { "name": groupName } ) ;
   }

   // check result
   if ( 0 != retArray.size() )
   {
      isExist = true ;
   }
   return isExist ;
}

System.prototype.killProcess = function( optionObj ) {

   // check argument
   if ( undefined == optionObj )
   {
      setLastErrMsg( "optionObj must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }
   if ( "object" != typeof( optionObj ) )
   {
      setLastErrMsg( "optionObj must be BsonObj" ) ;
      throw SDB_INVALIDARG ;
   }
   if ( undefined == optionObj.sig )
   {
      optionObj.sig = "term"
   }

   if ( undefined != this._remote )
   {
      this._remote._runCommand( "system kill process",
                                { "sig" : optionObj.sig,
                                  "pid": optionObj.pid } ) ;
   }
   else
   {
      System.killProcess( optionObj ) ;
   }
}

System.prototype.getProcUlimitConfigs = function() {
   var retObj ;

   // check argument
   if ( 0 < arguments.length )
   {
      setLastErrMsg( "getUlimitConfigs() should have non arguments" ) ;
      throw SDB_INVALIDARG ;
   }

   if ( undefined != this._remote )
   {
      retObj = this._remote._runCommand( "system get proc ulimit configs" ) ;
   }
   else
   {
      retObj = System.getProcUlimitConfigs() ;
   }
   return retObj ;
}

System.prototype.setProcUlimitConfigs = function( configsObj ) {

   // check argument
   if ( undefined == configsObj )
   {
      setLastErrMsg( "configsObj must be configs" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   if ( undefined != this._remote )
   {
      this._remote._runCommand( "system set proc ulimit configs", {},{},
                                { "configs": configsObj } ) ;
   }
   else
   {
      System.setProcUlimitConfigs( configsObj ) ;
   }
}

System.prototype.getUserEnv = function() {
   var result ;
   if ( undefined != this._remote )
   {
      result = this._remote._runCommand( "system get user env" ) ;
   }
   else
   {
      result = System.getUserEnv() ;
   }
   return result ;
}

System.prototype.getSystemConfigs = function( type ) {
   var retObj ;

   if ( undefined != this._remote )
   {
      if ( undefined != type )
      {
         retObj = this._remote._runCommand( "system get system configs",
                                            { "type": type } ) ;
      }
      else
      {
         retObj = this._remote._runCommand( "system get system configs" ) ;
      }
   }
   else
   {
      if ( undefined != type )
      {
         retObj = System.getSystemConfigs( type ) ;
      }
      else
      {
         retObj = System.getSystemConfigs( ) ;
      }
   }
   return retObj ;
}

System.prototype.runService = function ( serviceName, command, options ) {
   var result ;

   // check argument
   if ( undefined == serviceName )
   {
      setLastErrMsg( "serviceName must be config" ) ;
      throw SDB_INVALIDARG ;
   }
   if ( 'string' != typeof( serviceName ) )
   {
      setLastErrMsg( "serviceName must be string" ) ;
      throw SDB_INVALIDARG ;
   }

   if ( undefined == command )
   {
      setLastErrMsg( "command must be config" ) ;
      throw SDB_INVALIDARG ;
   }
   if ( 'string' != typeof( command ) )
   {
      setLastErrMsg( "command must be string" ) ;
      throw SDB_INVALIDARG ;
   }

   if ( undefined != this._remote )
   {
      var retObj ;
      if ( undefined != options )
      {
         retObj = this._remote._runCommand( "system run service",
                                            { "command": command,
                                              "options": options },
                                            { "serviceName": serviceName } ) ;
      }
      else
      {
         retObj = this._remote._runCommand( "system run service",
                                            { "command": command },
                                            { "serviceName": serviceName } ) ;
      }
      result = retObj.toObj().outStr ;
   }
   else
   {
      if ( "undefined" != options )
      {
         result = System.runService( serviceName, command, options ) ;
      }
      else
      {
         result = System.runService( serviceName, command ) ;
      }
   }
   return result ;
}

System.prototype.buildTrusty = function() {

   if ( "LINUX" == System.type() )
   {
      if ( undefined != this._remote )
      {
         var homeDir = System._getHomePath() ;
         System._createSshKey() ;
         var pubKey = File._readFile( homeDir + "/.ssh/id_rsa.pub" ) ;
         this._remote._runCommand( "system build trusty", {}, {},
                                   { "key": pubKey } ) ;
      }
   }
}

System.prototype.removeTrusty = function() {

   if ( "LINUX" == System.type() )
   {
      if ( undefined != this._remote )
      {
         var homeDir = System._getHomePath() ;
         var matchStr = File._readFile( homeDir + "/.ssh/id_rsa.pub" ) ;
         this._remote._runCommand( "system remove trusty", {}, {},
                                   { "matchStr": matchStr } ) ;
      }
   }
}

System.prototype.getPID = function() {

   // check argument
   if ( 0 < arguments.length )
   {
      setLastErrMsg( "No need arguments" ) ;
      throw SDB_INVALIDARG ;
   }

   var pid ;
   if ( undefined != this._remote )
   {
      pid = this._remote._runCommand( "system get pid" ).toObj().PID ;
   }
   else
   {
      pid = System.getPID() ;
   }
   return pid ;
}

System.prototype.getTID = function() {

   // check argument
   if ( 0 < arguments.length )
   {
      setLastErrMsg( "No need arguments" ) ;
      throw SDB_INVALIDARG ;
   }

   var tid ;
   if ( undefined != this._remote )
   {
      tid = this._remote._runCommand( "system get tid" ).toObj().TID ;
   }
   else
   {
      tid = System.getTID() ;
   }
   return tid ;
}

System.prototype.getEWD = function() {

   // check argument
   if ( 0 < arguments.length )
   {
      setLastErrMsg( "No need arguments" ) ;
      throw SDB_INVALIDARG ;
   }

   var ewd ;
   if ( undefined != this._remote )
   {
      ewd = this._remote._runCommand( "system get ewd" ).toObj().EWD ;
   }
   else
   {
      ewd = System.getEWD() ;
   }
   return ewd ;
}
// end System

// Cmd member function
Cmd.prototype.getInfo = function() {

   var result ;
   var infoObj ;
   if ( undefined != this._remote )
   {
      var _remoteInfo = this._remote.getInfo() ;
      infoObj = _remoteInfo.toObj() ;
      infoObj.isRemote = true ;
   }
   else
   {
      infoObj = new Object() ;
      infoObj.isRemote = false ;
   }
   result = this._getInfo( infoObj ) ;
   return result ;
}

Cmd.prototype.getCommand = function() {

   if ( undefined != this._remote )
   {
      return this._command ;
   }
   else
   {
      return this._getCommand() ;
   }
}

Cmd.prototype.getLastOut = function() {

   if ( undefined != this._remote )
   {
      return this._strOut ;
   }
   else
   {
      return this._getLastOut() ;
   }
}

Cmd.prototype.getLastRet = function() {

   if ( undefined != this._remote )
   {
      return this._retCode ;
   }
   else
   {
      return this._getLastRet() ;
   }
}

Cmd.prototype.run = function( cmd, args, timeout, useShell ) {
   var retStr ;

   // check argument
   if ( 1 > arguments.length )
   {
      setLastErrMsg( "cmd must be config" ) ;
      throw SDB_INVALIDARG ;
   }

   if ( "string" != typeof( cmd ) )
   {
      setLastErrMsg( "cmd must be string" ) ;
      throw SDB_INVALIDARG ;
   }
   this._command = cmd ;

   if ( 2 > arguments.length )
   {
      args = "" ;
   }
   else
   {
      if ( "string" != typeof( args ) )
      {
         setLastErrMsg( "environment should be a string" ) ;
         throw SDB_INVALIDARG ;
      }
      this._command += " " + args ;
   }

   if ( 3 > arguments.length )
   {
      timeout = 0 ;
   }
   if ( 4 > arguments.length )
   {
      useShell = 1 ;
   }

   // remote call
   if ( undefined != this._remote )
   {
      var retObj ;

      try
      {
         retObj = this._remote._runCommand( "cmd run",
                                            { "timeout": timeout,
                                              "useShell": useShell }, {},
                                            { "command": cmd,
                                              "args": args } ).toObj() ;
         this._retCode = getLastError() ;
         this._strOut = retObj.strOut ;
      }
      catch( e )
      {
         if( 0 < e )
         {
            var result = getLastErrObj().toObj() ;

            this._retCode = e ;

            if ( typeof( result.detail ) == 'string' )
            {
               this._strOut = result.detail ;
            }
            else
            {
               this._strOut = getLastErrMsg() ;
            }

            if( "" == this._strOut )
            {
               setLastErrMsg( "Run command(\"" + cmd +
                              "\") return code is " + e ) ;
            }
            else
            {
               setLastErrMsg( this._strOut ) ;
            }
         }
         throw e ;
      }

      retStr = this._strOut ;
   }
   else
   {
      if ( undefined != useShell )
      {
         retStr = this._run( cmd, args, timeout, useShell ) ;
      }
   }
   return retStr ;
}

Cmd.prototype.start = function( cmd, args, timeout, useShell ) {
   var retPid ;

   // check argument
   if ( 1 > arguments.length )
   {
      setLastErrMsg( "cmd must be config" ) ;
      throw SDB_INVALIDARG ;
   }

   if ( "string" != typeof( cmd ) )
   {
      setLastErrMsg( "cmd must be string" ) ;
      throw SDB_INVALIDARG ;
   }
   this._command = cmd ;

   if ( 2 > arguments.length )
   {
      args = "" ;
   }
   else
   {
      if ( "string" != typeof( args ) )
      {
         setLastErrMsg( "environment should be a string" ) ;
         throw SDB_INVALIDARG ;
      }
      this._command += " " + args ;
   }

   if ( 3 > arguments.length )
   {
      timeout = 100 ;
   }
   if ( 4 > arguments.length )
   {
      useShell = 1 ;
   }

   if ( undefined != this._remote )
   {
      var recvObj ;
      try
      {
         recvObj = this._remote._runCommand( "cmd start",
                                             { "useShell": useShell,
                                               "timeout": timeout }, {},
                                             { "command": cmd,
                                               "args": args } ).toObj() ;
         this._retCode = getLastError() ;
         this._strOut = recvObj.strOut ;
      }
      catch( e )
      {
         if( 0 <= e )
         {
            this._retCode = e ;
            this._strOut = getLastErrMsg() ;
         }
         throw e ;
      }
      retPid = recvObj.pid ;
   }
   else
   {
      retPid = this._start( cmd, args, timeout, useShell ) ;
   }
   return retPid ;
}

Cmd.prototype.runJS = function( code ) {
   var code ;

   // check argument
   if ( undefined == code )
   {
      setLastErrMsg( "code must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }
   if ( "string" != typeof( code ) )
   {
      setLastErrMsg( "code must be config" ) ;
      throw SDB_INVALIDARG ;
   }

   // check if is remote obj
   if ( undefined != this._remote )
   {
      var recvObj ;
      recvObj = this._remote._runCommand( "cmd run js", {}, {},
                                            { "code": code } ) ;
      return recvObj.toObj().strOut ;
   }
   else
   {
      setLastErrMsg( "runJS() should be called by remote cmd obj" ) ;
      throw SDB_SYS ;
   }
}
// end Cmd

// File static function
File.list = function( optionObj, filterObj ) {

   var retObj ;
   var objArr ;
   var result ;
   var displayMode = "obj" ;

   // check argument
   if ( undefined != optionObj )
   {
      if ( undefined != optionObj.displayMode )
      {
         displayMode = optionObj.displayMode ;
         delete optionObj.displayMode ;

      }
      retObj = File._list( optionObj ) ;
   }
   else
   {
      retObj = File._list() ;
   }

   if ( undefined != filterObj )
   {
      var filter = new _Filter( filterObj ) ;
      objArr = filter.match( retObj.toObj() ) ;
   }
   else
   {
      var filter = new _Filter( {} ) ;
      objArr =  filter.match( retObj.toObj() ) ;
   }

   if ( "text" == displayMode )
   {
      result = objArr._formatStr() ;
   }
   else
   {
      result = objArr ;
   }

   return result ;
}

File.isFile = function( pathname ) {

   // check argument
   if ( undefined == pathname )
   {
      setLastErrMsg( "pathname must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   // check return file type
   if ( "FIL" == File._getPathType( pathname ) )
   {
      return true ;
   }
   else
   {
      return false ;
   }
}

File.isDir = function( pathname ) {

   // check argument
   if ( undefined == pathname )
   {
      setLastErrMsg( "pathname must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   // check return file type
   if ( "DIR" == File._getPathType( pathname ) )
   {
      return true ;
   }
   else
   {
      return false ;
   }
}

File.find = function( optionObj, filterObj ) {
   var recvObj ;
   var retArr ;
   var result ;
   var displayMode = "obj" ;

   // check argument
   if ( undefined == optionObj )
   {
      setLastErrMsg( "optionObj must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   if ( undefined != optionObj.displayMode )
   {
      displayMode = optionObj.displayMode ;
      delete optionObj.displayMode ;
   }

   recvObj  = File._find( optionObj ) ;

   // filter
   if ( undefined != filterObj )
   {
      var filter = new _Filter( filterObj ) ;
      retArr = filter.match( recvObj.toObj() ) ;
   }
   else
   {
      var filter = new _Filter( {} ) ;
      retArr = filter.match( recvObj.toObj() ) ;
   }

   // set display format
   if ( "text" == displayMode )
   {
      result = retArr._formatStr() ;
   }
   else
   {
      result = retArr ;
   }
   return result ;
}

File.getUmask = function( base ) {

   var umaskStr = File._getUmask() ;
   var umask = parseInt( umaskStr, 8 ) ;
   if ( undefined != base )
   {
      if ( "string" == typeof( base ) )
      {
         base = parseInt( base ) ;
      }
      if ( 8 == base )
      {
         umask = ( "0000" + umask.toString( 8 ) ).substr( -4 ) ;
      }
      else if ( 10 == base )
      {
         umask = umask.toString( 10 ) ;
      }
      else if ( 16 == base )
      {
         umask = '0x' + umask.toString( 16 ) ;
      }
      else
      {
         setLastErrMsg( "base must be number( 8/10/16 )" ) ;
         throw SDB_INVALIDARG ;
      }
   }
   return umask ;
}

File.scp = function( src, dst, isReplace, mode ) {

   if( undefined == src )
   {
      setLastErrMsg( "src must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }
   if( "string" != typeof( src ) )
   {
      setLastErrMsg( "src must be string" ) ;
      throw SDB_INVALIDARG ;
   }

   if( undefined == dst )
   {
      setLastErrMsg( "dst must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }
   if( "string" != typeof( dst ) )
   {
      setLastErrMsg( "dst must be string" ) ;
      throw SDB_INVALIDARG ;
   }

   var srcFile ;
   var dstFile ;
   var COPY_UNIT = 4*1024*1024 ;
   var srcArr = src.split( "@" ) ;
   var dstFilename ;
   if( srcArr.length > 1 )
   {
      var hostPortSplit = srcArr[0].split( ":" ) ;
      var remote = new Remote( hostPortSplit[0], hostPortSplit[1] ) ;
      var fileMgr = remote.getFile() ;

      if( false == fileMgr.exist( srcArr[1] ) )
      {
         setLastErrMsg( "src not exist" ) ;
         throw SDB_FNE ;
      }
      if( undefined == mode )
      {
         mode = fileMgr._getPermission( srcArr[1] ) ;
      }
      srcFile = remote.getFile( srcArr[1], 0644, SDB_FILE_READONLY ) ;
   }
   else
   {
      if( false == File.exist( srcArr[0] ) )
      {
         setLastErrMsg( "src not exist" ) ;
         throw SDB_FNE ;
      }
      if( undefined == mode )
      {
         mode = File._getPermission( srcArr[0] ) ;
      }
      srcFile = new File( srcArr[0], 0644, SDB_FILE_READONLY ) ;
   }

   var dstArr = dst.split( "@" ) ;
   if( dstArr.length > 1 )
   {
      var hostPortSplit = dstArr[0].split( ":" ) ;
      var remote = new Remote( hostPortSplit[0], hostPortSplit[1] ) ;
      var fileMgr = remote.getFile() ;
      dstFilename = dstArr[1] ;
      if( true == fileMgr.exist( dstFilename ) )
      {
         if( false == isReplace )
         {
            setLastErrMsg( "dst exist" ) ;
            throw SDB_FE ;
         }
         else
         {
            dstFile = remote.getFile( dstFilename, mode,
                                      SDB_FILE_REPLACE| SDB_FILE_READWRITE ) ;
         }
      }
      else
      {
         dstFile = remote.getFile( dstFilename, mode,
                                   SDB_FILE_CREATEONLY | SDB_FILE_READWRITE ) ;
      }
   }
   else
   {
      dstFilename = dstArr[0] ;
      if( true == File.exist( dstFilename ) )
      {
         if( false == isReplace )
         {
            setLastErrMsg( "dst exist" ) ;
            throw SDB_FE ;
         }
         else
         {
            dstFile = new File( dstFilename, mode,
                                SDB_FILE_REPLACE | SDB_FILE_READWRITE ) ;
         }
      }
      else
      {
         dstFile = new File( dstFilename, mode,
                             SDB_FILE_CREATEONLY | SDB_FILE_READWRITE ) ;
      }
   }

   try
   {
      while( true )
      {
         var fileContent = srcFile.readContent( COPY_UNIT ) ;
         dstFile.writeContent( fileContent ) ;
         fileContent.clear() ;
      }
   }
   catch( e )
   {
      srcFile.close() ;
      dstFile.close() ;
      var errValue = e.message || e;
      if( -9 == errValue )
      {
         println( "Success to copy file from " + src + " to " + dst ) ;
      }
      else
      {
         throw e ;
      }
   }
}

// File member function
File.prototype.getInfo = function() {
   var result ;
   var infoObj ;
   if ( undefined != this._remote )
   {
      var _remoteInfo = this._remote.getInfo() ;
      infoObj = _remoteInfo.toObj() ;
      infoObj.isRemote = true ;
      infoObj.filename = this._filename ;
   }
   else
   {
      infoObj = new Object() ;
      infoObj.isRemote = false ;
   }
   result = this._getInfo( infoObj ) ;
   return result ;
}

File.prototype.read = function( size ) {
   var str ;

   if ( undefined != this._remote )
   {
      var retObj ;
      if ( undefined != size )
      {
         retObj = this._remote._runCommand( "file read", {},
                                            { "FID": this._FID },
                                            { "Size": size } ) ;
      }
      else
      {
         retObj = this._remote._runCommand( "file read", {},
                                            { "FID": this._FID } ) ;
      }
      var recvObj = retObj.toObj() ;
      str = recvObj.Content ;
   }
   else
   {
      if ( undefined != size )
      {
         str = this._read( size ) ;
      }
      else
      {
         str = this._read() ;
      }
   }
   return str ;
}

File.prototype.readContent = function( size )
{
   var retObj ;
   if ( undefined != this._remote )
   {
      if( undefined != size )
      {
         retObj = this._readContent( this._remote, this._FID, size ) ;
      }
      else
      {
         retObj = this._readContent( this._remote, this._FID ) ;
      }
   }
   else
   {
      if( undefined != size )
      {
         retObj = this._readContent( size ) ;
      }
      else
      {
         retObj = this._readContent() ;
      }
   }
   return retObj ;
}

File.prototype.write = function( content ){

   if ( undefined != this._remote )
   {
      this._remote._runCommand( "file write", {}, { "FID": this._FID },
                                { "Content": content } ) ;
   }
   else
   {
      this._write( content ) ;
   }
}

File.prototype.writeContent = function( content )
{
   if( undefined != this._remote )
   {
      this._writeContent( this._remote, this._FID, content ) ;
   }
   else
   {
      this._writeContent( content ) ;
   }
}

File.prototype.truncate = function( size ) {
   if ( undefined != this._remote )
   {
      if ( undefined === size )
      {
         this._remote._runCommand( "file truncate", {}, { "FID": this._FID } ) ;
      }
      else
      {
         this._remote._runCommand( "file truncate", {}, { "FID": this._FID },
                                             { "Size": size } ) ;
      }
   }
   else
   {
      if ( undefined === size )
      {
         this._truncate() ;
      }
      else
      {
         this._truncate( size ) ;
      }
   }
}

File.prototype.seek = function( offset, where ) {

   // check argument
   if ( undefined == offset )
   {
      setLastErrMsg( "Offset must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   var optionObj = {} ;
   if ( undefined != where )
   {
      optionObj.where = where ;
   }

   if ( undefined != this._remote )
   {
      this._remote._runCommand( "file seek", optionObj, { "FID": this._FID },
                                { "SeekSize": offset } ) ;
   }
   else
   {
      this._seek( offset, optionObj ) ;
   }
}

File.prototype.tellPosition = function ()
{
   var result ;
   if ( undefined != this._remote )
   {
      var recvObj = this._remote._runCommand( "file tell position", {}, { "FID": this._FID }, {} ) ;
      result = recvObj.toObj().Position ;
   }
   else
   {
      result = this._tellPosition() ;
   }

   return result ;
}

File.prototype.close = function() {
   if ( undefined == this._remote )
   {
      this._close() ;
   }
   else
   {
      if( this._FID != undefined )
      {
         this._remote._runCommand( "file close", {}, { "FID": this._FID } ) ;
      }
   }
}

File.prototype.remove = function( filepath ) {

   // check argument
   if ( undefined == filepath )
   {
      setLastErrMsg( "filepath must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   if ( undefined != this._remote )
   {
      this._remote._runCommand( "file remove", {}, {},
                               { "Pathname" : filepath } ) ;
   }
   else
   {
      File.remove( filepath ) ;
   }
}

File.prototype.exist = function( filepath ) {
   var isExist ;

   // check argument
   if ( undefined == filepath )
   {
      setLastErrMsg( "filepath must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   if ( undefined != this._remote )
   {
      var recvObj = this._remote._runCommand( "file is exist", {}, {},
                                              { "Pathname" : filepath } ) ;
      isExist = recvObj.toObj().IsExist ;
   }
   else
   {
      isExist = File.exist( filepath ) ;
   }
   return isExist ;
}

File.prototype.copy = function( src, dst, replace, mode ) {

   // check argument
   if ( undefined == src )
   {
      setLastErrMsg( "Src must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }
   if ( undefined == dst )
   {
      setLastErrMsg( "Dst must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   if ( undefined != this._remote )
   {
      var optionObj = {} ;
      if ( undefined != mode )
      {
         optionObj.mode = mode ;
      }
      if ( undefined != replace )
      {
         optionObj.isReplace = replace ;
      }
      this._remote._runCommand( "file copy", optionObj,
                                { "Src": src }, { "Dst": dst } ) ;
   }
   else
   {
      if ( undefined != mode )
      {
         File.copy( src, dst, replace, mode ) ;
      }
      else if ( undefined != replace )
      {
         File.copy( src, dst, replace ) ;
      }
      else
      {
         File.copy( src, dst ) ;
      }
   }
}

File.prototype.move = function( src, dst ) {

   // check argument
   if ( undefined == src )
   {
      setLastErrMsg( "Src must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }
   if ( undefined == dst )
   {
      setLastErrMsg( "Dst must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   if ( undefined != this._remote )
   {
      this._remote._runCommand( "file move", {}, { "Src": src },
                              { "Dst": dst } ) ;
   }
   else
   {
      File.move( src, dst ) ;
   }
}

File.prototype.mkdir = function( name, mode ) {

   // check argument
   if ( undefined == name )
   {
      setLastErrMsg( "Name must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   if ( undefined != this._remote )
   {
      var optionObj = {} ;
      if( undefined != mode )
      {
         optionObj.mode = mode ;
      }
      this._remote._runCommand( "file mkdir", optionObj, {},
                               { "Dirname": name } ) ;
   }
   else
   {
      if( undefined != mode )
      {
         File.mkdir( name, mode ) ;
      }
      else
      {
         File.mkdir( name ) ;
      }
   }
}

File.prototype.find = function( optionObj, filterObj ) {

   // check argument
   if ( undefined == optionObj )
   {
      setLastErrMsg( "OptionObj must be config" ) ;
      throw SDB_OUT_OF_BOUND  ;
   }
   if ( false == optionObj instanceof Object )
   {
      setLastErrMsg( "OptionObj must be Object" ) ;
      throw SDB_INVALIDARG  ;
   }

   var result ;
   if ( undefined != this._remote )
   {
      var matchResult ;
      var displayMode = "obj" ;
      if ( undefined != optionObj.displayMode )
      {
         displayMode = optionObj.displayMode ;
         delete optionObj.displayMode ;
      }

      var recvObj = this._remote._runCommand( "file find", optionObj ).toObj() ;

      // filter
      if ( undefined != filterObj )
      {
         var filter = new _Filter( filterObj ) ;
         matchResult = filter.match( recvObj ) ;
      }
      else
      {
         var filter = new _Filter( {} ) ;
         matchResult = filter.match( recvObj ) ;
      }

      // set display format
      if ( "text" == displayMode )
      {
         result = matchResult._formatStr() ;
      }
      else
      {
         result = matchResult ;
      }
   }
   else
   {
      result = File.find( optionObj, filterObj ) ;
   }

   return result ;
}

File.prototype.chmod = function( filename, mode, recursive ) {

   if ( undefined != this._remote )
   {
      var optionObj = {} ;
      if ( undefined != recursive )
      {
         optionObj.isRecursive = recursive ;
      }
      this._remote._runCommand( "file chmod", optionObj, { "Pathname": filename },
                                { "Mode": mode } ) ;
   }
   else
   {
      if ( undefined != recursive )
      {
         File.chmod( filename, mode, recursive ) ;
      }
      else
      {
         File.chmod( filename, mode ) ;
      }
   }
}

File.prototype.toString = function()
{
   var result ;
   if ( undefined != this._remote )
   {
      if ( undefined != this._filename )
      {
         result = this._filename ;
      }
      else
      {
         result = "" ;
      }
   }
   else
   {
      result = this._toString() ;
   }
   return result ;
}

File.prototype.chown = function( filename, optionObj, recursive ) {
   if ( undefined != this._remote )
   {
      if ( undefined != recursive )
      {
         this._remote._runCommand( "file chown", { "isRecursive": recursive },
                                   { "Pathname": filename }, optionObj ) ;
      }
      else
      {
         this._remote._runCommand( "file chown", {}, { "Pathname": filename },
                                  optionObj ) ;
      }
   }
   else
   {
      if ( undefined != recursive )
      {
         File.chown( filename, optionObj, recursive ) ;
      }
      else
      {
         File.chown( filename, optionObj ) ;
      }
   }
}

File.prototype.chgrp = function( filename, groupname, recursive ) {
   if ( undefined != this._remote )
   {
      if ( undefined != recursive )
      {
         this._remote._runCommand( "file chgrp", { "isRecursive": recursive },
                                   { "Pathname": filename },
                                   { "Groupname": groupname } ) ;
      }
      else
      {
         this._remote._runCommand( "file chgrp", {}, { "Pathname": filename },
                                  { "Groupname": groupname } ) ;
      }
   }
   else
   {
      if ( undefined != recursive )
      {
         File.chgrp( filename, groupname, recursive ) ;
      }
      else
      {
         File.chgrp( filename, groupname ) ;
      }
   }
}

File.prototype.setUmask = function( mask ) {

   // check argument
   if ( undefined == mask )
   {
      setLastErrMsg( "Mask must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   if ( undefined != this._remote )
   {
      this._remote._runCommand( "file set umask", {}, {}, { "Mask": mask } ) ;
   }
   else
   {
      File.setUmask( mask ) ;
   }
}

File.prototype.getUmask = function( base ) {
   var result ;

   if ( undefined != this._remote )
   {
      var recvObj = this._remote._runCommand( "file get umask" ) ;
      var umask = parseInt( recvObj.toObj().Mask, 8 ) ;
      if ( undefined != base )
      {
         if ( "string" == typeof( base ) )
         {
            base = parseInt( base ) ;
         }
         if ( 8 == base )
         {
            umask = ( "0000" + umask.toString( 8 ) ).substr( -4 ) ;
         }
         else if ( 10 == base )
         {
            umask = umask.toString( 10 ) ;
         }
         else if ( 16 == base )
         {
            umask = "0x" + umask.toString( 16 ) ;
         }
         else
         {
            setLastErrMsg( "base must be number( 8/10/16 )" ) ;
            throw SDB_INVALIDARG ;
         }
      }
      result = umask ;
   }
   else
   {
      result = File.getUmask( base ) ;
   }
   return result ;
}

File.prototype.list = function( optionObj, filterObj ) {
   var result ;
   if ( undefined == optionObj )
   {
      optionObj = {} ;
   }

   if ( undefined != this._remote )
   {
      var displayMode = "obj" ;
      if ( undefined != optionObj.displayMode )
      {
         displayMode = optionObj.displayMode ;
         delete optionObj.displayMode ;
      }

      var recvObj = this._remote._runCommand( "file list", optionObj ).toObj() ;
      var matchResult ;
      if ( undefined != filterObj )
      {
         var filter = new _Filter( filterObj ) ;
         matchResult = filter.match( recvObj ) ;
      }
      else
      {
         var filter = new _Filter( {} ) ;
         matchResult = filter.match( recvObj ) ;
      }

      if ( "text" == displayMode )
      {
         result = matchResult._formatStr() ;
      }
      else
      {
         result = matchResult ;
      }
   }
   else
   {
      result = File.list( optionObj, filterObj ) ;
   }
   return result ;
}

File.prototype.isFile = function( pathname ) {

   // check argument
   if ( undefined == pathname )
   {
      setLastErrMsg( "mask must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   var result = false ;
   if ( undefined != this._remote )
   {
      var recvObj = this._remote._runCommand( "file get path type", {},
                                             { "Pathname":pathname } ) ;
      if ( "FIL" == recvObj.toObj().PathType )
      {
         result = true ;
      }
   }
   else
   {
      result = File.isFile( pathname ) ;
   }
   return result ;
}

File.prototype.isDir = function( pathname ) {

   // check argument
   if ( undefined == pathname )
   {
      setLastErrMsg( "mask must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   var result = false ;
   if ( undefined != this._remote )
   {
      var recvObj = this._remote._runCommand( "file get path type", {},
                                             { "Pathname":pathname } ) ;
      if ( "DIR" == recvObj.toObj().PathType )
      {
         result = true ;
      }
   }
   else
   {
      result = File.isDir( pathname ) ;
   }
   return result ;
}

File.prototype.isEmptyDir = function( pathname ) {

   // check argument
   if ( undefined == pathname )
   {
      setLastErrMsg( "Pathname must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   var result ;
   if ( undefined != this._remote )
   {
      var recvObj = this._remote._runCommand( "file is empty dir", {},
                                             { "Pathname":pathname } ) ;

      var isEmpty = recvObj.toObj().IsEmpty ;
      if ( isEmpty )
      {
         result = true ;
      }
      else
      {
         result = false ;
      }
   }
   else
   {
      result = File.isEmptyDir( pathname ) ;
   }
   return result ;
}

File.prototype.stat = function( filename ) {

   // check argument
   if ( undefined == filename )
   {
      setLastErrMsg( "Filename must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   var result ;
   if ( undefined != this._remote )
   {
      result = this._remote._runCommand( "file stat", {},
                                        { "Filename": filename } ) ;
   }
   else
   {
      result = File.stat( filename ) ;
   }
   return result ;
}

File.prototype.md5 = function( filename ) {

   // check argument
   if ( undefined == filename )
   {
      setLastErrMsg( "Filename must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }

   var result ;
   if ( undefined != this._remote )
   {
      var recvObj = this._remote._runCommand( "file md5", {},
                                              { "Filename": filename } ) ;
      result = recvObj.toObj().MD5 ;
   }
   else
   {
      result = File.md5( filename ) ;
   }
   return result ;

}

File.prototype._getPermission = function( pathname ) {

   if( undefined == pathname )
   {
      setLastErrMsg( "Pathname must be config" ) ;
      throw SDB_OUT_OF_BOUND ;
   }
   else if( "string" != typeof( pathname ) )
   {
      setLastErrMsg( "Pathname must be string" ) ;
      throw SDB_INVALIDARG ;
   }

   if( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "file get permission", {}, {},
                                             { "Pathname": pathname } ).toObj() ;
      return retObj.Permission ;
   }
   else
   {
      return File._getPermission( pathname ) ;
   }
}

File.prototype.getSize = function( filename ) {
   if( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "file get content size", {},
                                             { "Filename": filename } ).toObj() ;
      return retObj.Size ;
   }
   else
   {
      return File.getSize( filename ) ;
   }
}

File.prototype.readLine = function() {
   var retStr ;
   if( undefined != this._remote )
   {
      var retObj = this._remote._runCommand( "file read line", {},
                                             { "FID": this._FID } ) ;
      retStr = retObj.toObj().Content ;
   }
   else
   {
      retStr = this._readLine() ;
   }
   return retStr ;
}

// end File

// IniFile member function
IniFile.prototype.setValue = function( argv1, argv2, argv3 ) {
   var section, key, value ;
   var argc = arguments.length ;

   if ( 2 > argc )
   {
      setLastErrMsg( "Missing argument" ) ;
      throw SDB_INVALIDARG ;
   }
   else if ( 2 == argc )
   {
      section = "" ;
      key     = argv1 ;
      value   = argv2 ;
   }
   else
   {
      section = argv1 ;
      key     = argv2 ;
      value   = argv3 ;
   }

   this._setValue( section, key, value ) ;
}

IniFile.prototype.getValue = function( argv1, argv2 ) {
   var section, key ;
   var argc = arguments.length ;

   if ( 1 > argc )
   {
      setLastErrMsg( "Missing argument" ) ;
      throw SDB_INVALIDARG ;
   }
   else if ( 1 == argc )
   {
      section = "" ;
      key     = argv1 ;
   }
   else
   {
      section = argv1 ;
      key     = argv2 ;
   }

   return this._getValue( section, key ) ;
}

IniFile.prototype.setSectionComment = function( section, comment ) {
   var argc = arguments.length ;

   if ( 1 > argc )
   {
      setLastErrMsg( "Missing argument section" ) ;
      throw SDB_INVALIDARG ;
   }
   else if ( 2 > argc )
   {
      setLastErrMsg( "Missing argument comment" ) ;
      throw SDB_INVALIDARG ;
   }

   comment = this._convertComment( comment ) ;

   this._setSectionComment( section, comment ) ;
}

IniFile.prototype.getSectionComment = function( section ) {
   var argc = arguments.length ;

   if ( 1 > argc )
   {
      setLastErrMsg( "Missing argument section" ) ;
      throw SDB_INVALIDARG ;
   }

   var comment = this._getSectionComment( section ) ;

   if ( typeof( comment ) == 'string' )
   {
      comment = this._comment2String( comment ) ;
   }

   return comment ;
}

IniFile.prototype.addSectionComment = function( section, comment ) {
   var argc  = arguments.length ;
   var flags = this._getFlags() ;
   var newComment ;

   if ( 1 > argc )
   {
      setLastErrMsg( "Missing argument section" ) ;
      throw SDB_INVALIDARG ;
   }
   else if ( 2 > argc )
   {
      setLastErrMsg( "Missing argument comment" ) ;
      throw SDB_INVALIDARG ;
   }

   newComment = this._getSectionComment( section ) ;
   if ( undefined === newComment )
   {
      newComment = "" ;
   }

   if ( newComment.length > 0 )
   {
      newComment += '\n' ;
   }

   newComment += this._convertComment( comment ) ;

   this._setSectionComment( section, newComment ) ;
}

IniFile.prototype.delSectionComment = function( section ) {
   var argc = arguments.length ;

   if ( 1 > argc )
   {
      setLastErrMsg( "Missing argument section" ) ;
      throw SDB_INVALIDARG ;
   }

   this._setSectionComment( section, "" ) ;
}

IniFile.prototype.setComment = function( argv1, argv2, argv3, argv4 ) {
   var section, key, comment, pos ;
   var argc = arguments.length ;

   pos = true ;

   if ( 2 > argc )
   {
      setLastErrMsg( "Missing argument" ) ;
      throw SDB_INVALIDARG ;
   }
   else if ( 2 == argc )
   {
      section = "" ;
      key     = argv1 ;
      comment = argv2 ;
   }
   else if ( 3 == argc )
   {
      if ( typeof( argv3 ) == 'string' )
      {
         section = argv1 ;
         key     = argv2 ;
         comment = argv3 ;
      }
      else
      {
         section = "" ;
         key     = argv1 ;
         comment = argv2 ;
         pos     = argv3 ;
      }
   }
   else
   {
      section = argv1 ;
      key     = argv2 ;
      comment = argv3 ;
      pos     = argv4 ;
   }

   comment = this._convertComment( comment ) ;

   this._setComment( section, key, comment, pos ) ;
}

IniFile.prototype.getComment = function( argv1, argv2, argv3 ) {
   var section, key, pos ;
   var argc = arguments.length ;

   pos = true ;

   if ( 1 > argc )
   {
      setLastErrMsg( "Missing argument" ) ;
      throw SDB_INVALIDARG ;
   }
   else if ( 1 == argc )
   {
      section = "" ;
      key     = argv1 ;
   }
   else if ( 2 == argc )
   {
      if ( typeof( argv2 ) == 'string' )
      {
         section = argv1 ;
         key     = argv2 ;
      }
      else
      {
         section = "" ;
         key     = argv1 ;
         pos     = argv2 ;
      }
   }
   else
   {
      section = argv1 ;
      key     = argv2 ;
      pos     = argv3 ;
   }

   var comment = this._getComment( section, key, pos ) ;

   if ( typeof( comment ) == 'string' )
   {
      comment = this._comment2String( comment ) ;
   }

   return comment ;
}

IniFile.prototype.addComment = function( argv1, argv2, argv3, argv4 ) {
   var section, key, comment, pos ;
   var argc  = arguments.length ;
   var newComment ;

   pos = true ;

   if ( 2 > argc )
   {
      setLastErrMsg( "Missing argument section" ) ;
      throw SDB_INVALIDARG ;
   }
   else if ( 2 == argc )
   {
      section = "" ;
      key     = argv1 ;
      comment = argv2 ;
   }
   else if ( 3 == argc )
   {
      if ( typeof( argv3 ) == 'string' )
      {
         section = argv1 ;
         key     = argv2 ;
         comment = argv3 ;
      }
      else
      {
         section = "" ;
         key     = argv1 ;
         comment = argv2 ;
         pos     = argv3 ;
      }
   }
   else
   {
      section = argv1 ;
      key     = argv2 ;
      comment = argv3 ;
      pos     = argv4 ;
   }

   newComment = this._getComment( section, key, pos ) ;
   if ( undefined === newComment )
   {
      newComment = "" ;
   }

   if ( pos )
   {
      if ( newComment.length > 0 )
      {
         newComment += '\n' ;
      }

      newComment += this._convertComment( comment ) ;
   }
   else
   {
      if ( newComment.length > 0 )
      {
         newComment += ' ' + comment ;
      }
      else
      {
         newComment = this._convertComment( comment ) ;
      }
   }

   this._setComment( section, key, newComment, pos ) ;
}

IniFile.prototype.delComment = function( argv1, argv2, argv3 ) {
   var section, key, pos ;
   var argc = arguments.length ;

   pos = true ;

   if ( 1 > argc )
   {
      setLastErrMsg( "Missing argument" ) ;
      throw SDB_INVALIDARG ;
   }
   else if ( 1 == argc )
   {
      section = "" ;
      key     = argv1 ;
   }
   else if ( 2 == argc )
   {
      if ( typeof( argv2 ) == 'string' )
      {
         section = argv1 ;
         key     = argv2 ;
      }
      else
      {
         section = "" ;
         key     = argv1 ;
         pos     = argv2 ;
      }
   }
   else
   {
      section = argv1 ;
      key     = argv2 ;
      pos     = argv3 ;
   }

   this._setComment( section, key, "", pos ) ;
}

IniFile.prototype.setLastComment = function( comment ) {

   comment = this._convertComment( comment ) ;

   this._setLastComment( comment ) ;
}

IniFile.prototype.getLastComment = function() {
   var comment = this._getLastComment() ;

   if ( typeof( comment ) == 'string' )
   {
      comment = this._comment2String( comment ) ;
   }

   return comment ;
}

IniFile.prototype.addLastComment = function( comment ) {
   var newComment ;

   newComment = this._getLastComment() ;

   newComment = newComment + '\n' + this._convertComment( comment ) ;

   return this._setLastComment( newComment ) ;
}

IniFile.prototype.delLastComment = function() {
   this._setLastComment( "" ) ;
}

IniFile.prototype.enableItem = function( argv1, argv2 ) {
   var section, key ;
   var argc = arguments.length ;

   if ( 1 > argc )
   {
      setLastErrMsg( "Missing argument" ) ;
      throw SDB_INVALIDARG ;
   }
   else if ( 1 == argc )
   {
      section = "" ;
      key     = argv1 ;
   }
   else
   {
      section = argv1 ;
      key     = argv2 ;
   }

   this._enableItem( section, key ) ;
}

IniFile.prototype.disableItem = function( argv1, argv2 ) {
   var section, key ;
   var argc = arguments.length ;

   if ( 1 > argc )
   {
      setLastErrMsg( "Missing argument" ) ;
      throw SDB_INVALIDARG ;
   }
   else if ( 1 == argc )
   {
      section = "" ;
      key     = argv1 ;
   }
   else
   {
      section = argv1 ;
      key     = argv2 ;
   }

   this._disableItem( section, key ) ;
}

IniFile.prototype.disableAllItem = function() {
   this._disableAllItem() ;
}

IniFile.prototype.toString = function() {
   return this._toString() ;
}

IniFile.prototype.toObj = function() {
   return this._toObj() ;
}

IniFile.prototype.save = function() {
   if ( undefined != this._remote )
   {
      var file = this._remote.getFile( this._getFileName() ) ;
      var content = '' ;

      try
      {
         content = this._toString() ;
      }
      catch( e )
      {
         if ( typeof( e ) == 'number' )
         {
            var msg = getLastErrMsg() ;
            var result = {
               'errno': e,
               'description': msg,
               'detail': msg
            } ;
            setLastErrObj( result ) ;
         }

         throw e ;
      }

      file.truncate() ;

      file.write( content ) ;

      file.close() ;
   }
   else
   {
      return this._save() ;
   }
}

// end IniFile

// IniFile member function

function DiagLog(argv1, argv2, argv3, argv4) {
   var argc = arguments.length ;
   this.hostname   = 'localhost' ;
   this.svcname    = 11810 ;
   this.user       = '' ;
   this.password   = '' ;
   this.cipherUser = '' ;
   if ( 1 > argc )
   {
      // may be not need login
   }
   else if ( 1 == argc )
   {
      setLastErrMsg( "Missing argument" ) ;
      throw SDB_INVALIDARG ;
   }
   else if ( 2 == argc )
   {
      this.hostname   = argv1 ;
      this.svcname    = argv2 ;
   }
   else if ( 3 == argc )
   {
      this.hostname   = argv1 ;
      this.svcname    = argv2 ;
      this.cipherUser = argv3 ;
      if ( "object" != typeof( this.cipherUser ) )
      {
         setLastErrMsg( "cipherUser must be object" ) ;
         throw SDB_INVALIDARG ;
      }
   } else
   {
      this.hostname   = argv1 ;
      this.svcname    = argv2 ;
      this.user       = argv3 ;
      this.password   = argv4 ;
   }

   this._logTool = System.getEWD() + '/../tools/diaglog/logSearchTool.sh' ;
   this._tmpDir = "/tmp/sequoiadb" ;

   this.reset = function() {
      this._lastFile = '' ;
      this._lastest = '' ;
      this._path = '' ;
      this._timeBegin = '' ;
      this._timeEnd = '' ;
      this._output = '' ;
      this._error = '' ;
      this._diaglevel = '' ;
      this._keypattern = '' ;
      this._pid = '' ;
      this._tid = '' ;
      this._limit = 100 ;
      this._after = '' ;
      this._before = '' ;
      this._original = false ;
      this._snapshot = '' ;
      this._core = false ;
      this._trap = false ;
      this._compress = 'tar.gz' ;
      // internal
      this._showHelp = false ;
      this._needSearch = false ;
      this._role = '' ;
      this._serviceName = '' ;
      this._hostName = '' ;
      this._fileOnly = false ;
      this._nohup = false ;
      this._logFile = '' ;
      this._locationHostName = '' ;
      this._locationServiceName = '' ;
      this._locationNodeName = '' ;
      this._locationGroupName = '' ;
      this._locationRole = '' ;
      this._locationNodeID = '' ;
      this._locationGroupID = '' ;
   }
   this.reset() ;
}
DiagLog() ;

DiagLog.prototype.run = function() {
   return this._exec() ;
}

DiagLog.prototype.valueOf = function() {
   return this._exec() ;
}

DiagLog.prototype.toString = function() {
   return this._exec() ;
}

DiagLog.prototype._exec = function() {
   if ( this._showHelp )
   {
      this._showHelp = false ;
      return '' ;
   }

   var rc = '' ;
   try
   {
      if ( ! File.exist( this._logTool ) || File.isDir( this._logTool ) )
      {
         setLastErrMsg( this._logTool + ' does not exist' ) ;
         throw SDB_FNE ;
      }
   } catch ( e )
   {
      throw e ;
   }
   try
   {
      // save some parameters
      var savePath = this._path ;
      var saveOutput = this._output ;
      var saveOriginal = this._original ;
      var saveLogTool = this._logTool ;
      switch( this.mode )
      {
         case "search":
            this.close() ;
            rc = this._search() ;
            break ;
         case "collect":
            this.close() ;
            rc = this._collect() ;
            break ;
         case "analyze":
            rc = this._analyze() ;
            break ;
         default:
            break ;
      }
      // restore parameters
      this._path = savePath ;
      this._output = saveOutput ;
      this._original = saveOriginal ;
      this._logTool = saveLogTool ;
   } catch ( e )
   {
      throw e;
   }
   return rc ;
}

DiagLog.prototype._location = function( locationObj ) {
   if ( "object" != typeof( locationObj ) )
   {
      return ;
   } else
   {
      // reset
      this._locationHostName = '' ;
      this._locationNodeName = '' ;
      this._locationServiceName = '' ;
      this._locationGroupName = '' ;
      this._locationRole = '' ;
      this._locationNodeID = '' ;
      this._locationGroupID = '' ;
   }
   var keys = Object.keys( locationObj ) ;
   for ( var i = 0; i < keys.length; i++ )
   {
      var key = keys[i] ;
      var value = locationObj[key] ;
      switch( key )
      {
         case "HostName":
            this._locationHostName = value ;
            break ;
         case "NodeName":
            this._locationNodeName = value ;
            break ;
         case "ServiceName":
            this._locationServiceName = value ;
            break ;
         case "GroupName":
            this._locationGroupName = value ;
            break ;
         case "Role":
            this._locationRole = value ;
            break ;
         case "NodeID":
            this._locationNodeID = value ;
            break ;
         case "GroupID":
            this._locationGroupID = value ;
            break ;
         default:
            // do nothing
            break ;
      }
   }
}

DiagLog.prototype.search = function( locationObj ) {
   this.mode = "search" ;
   this._location( locationObj ) ;
   return this ;
}

DiagLog.prototype.collect = function( locationObj ) {
   this.mode = "collect" ;
   this._location( locationObj ) ;
   return this ;
}

DiagLog.prototype.analyze = function() {
   this.mode = "analyze" ;
   return this ;
}

DiagLog.prototype.lastFile = function( lastFile ) {
   if ( "number" != typeof( lastFile ) )
   {
      setLastErrMsg( "lastFile value must be a number" ) ;
      throw SDB_INVALIDARG ;
   } else if ( lastFile <= 0 )
   {
      setLastErrMsg( "lastFile value must be a number greater than 0" ) ;
      throw SDB_INVALIDARG ;
   }
   this._lastFile = lastFile ;
   return this ;
}

DiagLog.prototype.lastest = function( lastest ) {
   if ( "number" != typeof( lastest ) )
   {
      setLastErrMsg( "lastest value must be number" ) ;
      throw SDB_INVALIDARG ;
   } else if ( lastest <= 0 )
   {
      setLastErrMsg( "lastest value must be greater than 0" ) ;
      throw SDB_INVALIDARG ;
   }
   this._lastest = lastest ;
   return this ;
}

DiagLog.prototype.path = function( path ) {
   if ( "string" != typeof( path ) )
   {
      setLastErrMsg( "path must be string" ) ;
      throw SDB_INVALIDARG ;
   } else if ( "" == path )
   {
      setLastErrMsg( "path cannot be empty" ) ;
      throw SDB_INVALIDARG ;
   } else if ( ! /^\//.test( path ) ) {
      setLastErrMsg( "path must be an absolute path" ) ;
      throw SDB_INVALIDARG ;
   }
   this._path = path ;
   return this ;
}

DiagLog.prototype.timeBegin = function( timeBegin ) {
   if ( "string" != typeof( timeBegin ) )
   {
      setLastErrMsg( "timeBegin value must be string" ) ;
      throw SDB_INVALIDARG ;
   }
   if ( ! Date.parse(timeBegin) )
   {
      setLastErrMsg( "timeBegin value must be parsed with Date.parse()" ) ;
      throw SDB_INVALIDARG ;
   }
   this._timeBegin = new Date( new Date( timeBegin ).getTime() + 8 * 60 * 60 * 1000 ).toJSON().replace('Z', '').replace( 'T', '-' ) ;
   if ( '' != this._timeEnd && this._timeBegin >= this._timeEnd )
   {
      setLastErrMsg( "timeEnd value must be greater than timeBegin" ) ;
      throw SDB_INVALIDARG ;
   }
   return this ;
}

DiagLog.prototype.timeEnd = function( timeEnd ) {
   if ( "string" != typeof( timeEnd ) )
   {
      setLastErrMsg( "timeEnd value must be string" ) ;
      throw SDB_INVALIDARG ;
   }
   if ( ! Date.parse(timeEnd) )
   {
      setLastErrMsg( "timeEnd value must be parsed with Date.parse()" ) ;
      throw SDB_INVALIDARG ;
   }
   this._timeEnd = new Date( new Date( timeEnd ).getTime() + 8 * 60 * 60 * 1000 ).toJSON().replace('Z', '').replace( 'T', '-' ) ;
   if ( '' != this._timeBegin && this._timeBegin >= this._timeEnd )
   {
      setLastErrMsg( "timeEnd value must be greater than timeBegin" ) ;
      throw SDB_INVALIDARG ;
   }
   return this ;
}

DiagLog.prototype.output = function( output ) {
   if ( "string" != typeof( output ) )
   {
      setLastErrMsg( "output path must be string" ) ;
      throw SDB_INVALIDARG ;
   } else if ( "" == output )
   {
      setLastErrMsg( "output path cannot be empty" ) ;
      throw SDB_INVALIDARG ;
   } else if ( ! /^\//.test( output ) ) {
      setLastErrMsg( "output path must be an absolute path" ) ;
      throw SDB_INVALIDARG ;
   }
   this._output = output + '/result' ;
   return this ;
}

DiagLog.prototype.error = function( error ) {
   if ( "number" != typeof( error ) )
   {
      setLastErrMsg( "error code must be number" ) ;
      throw SDB_INVALIDARG ;
   } else if ( error >= 0 )
   {
      setLastErrMsg( "error code must be less than 0" ) ;
      throw SDB_INVALIDARG ;
   }
   this._error = error ;
   this._needSearch = true ;
   return this ;
}

DiagLog.prototype.diaglevel = function( diaglevel ) {
   if ( "number" != typeof( diaglevel ) )
   {
      setLastErrMsg( "diaglevel value must be 0-4" ) ;
      throw SDB_INVALIDARG ;
   } else if ( diaglevel < 0 || diaglevel > 4 )
   {
      setLastErrMsg( "diaglevel value must be 0-4" ) ;
      throw SDB_INVALIDARG ;
   }
   this._diaglevel = diaglevel ;
   return this ;
}

DiagLog.prototype.keypattern = function( keypattern ) {
   if ( "string" != typeof( keypattern ) )
   {
      setLastErrMsg( "keypattern value must be string" ) ;
      throw SDB_INVALIDARG ;
   } else if ( "" == keypattern )
   {
      setLastErrMsg( "keypattern value cannot be empty" ) ;
      throw SDB_INVALIDARG ;
   }
   this._keypattern = keypattern ;
   this._needSearch = true ;
   return this ;
}

DiagLog.prototype.pid = function( pid ) {
   if ( "number" != typeof( pid ) )
   {
      setLastErrMsg( "pid value must be number" ) ;
      throw SDB_INVALIDARG ;
   } else if ( pid <= 0 )
   {
      setLastErrMsg( "pid value must be greater than 0" ) ;
      throw SDB_INVALIDARG ;
   }
   this._pid = pid ;
   this._needSearch = true ;
   return this ;
}

DiagLog.prototype.tid = function( tid ) {
   if ( "number" != typeof( tid ) )
   {
      setLastErrMsg( "tid value must be number" ) ;
      throw SDB_INVALIDARG ;
   } else if ( tid <= 0 )
   {
      setLastErrMsg( "tid value must be greater than 0" ) ;
      throw SDB_INVALIDARG ;
   }
   this._tid = tid ;
   this._needSearch = true ;
   return this ;
}

DiagLog.prototype.limit = function( limit ) {
   if ( "number" != typeof( limit ) )
   {
      setLastErrMsg( "limit value must be number" ) ;
      throw SDB_INVALIDARG ;
   } else if ( 0 == limit )
   {
      setLastErrMsg( "limit value cannot be equal to 0" ) ;
      throw SDB_INVALIDARG ;
   }  else if ( -1 > limit )
   {
      setLastErrMsg( "limit value must be greater than 0, or -1" ) ;
      throw SDB_INVALIDARG ;
   }
   this._limit = limit ;
   return this ;
}

DiagLog.prototype.after = function( after ) {
   if ( "number" != typeof( after ) )
   {
      setLastErrMsg( "after value must be number" ) ;
      throw SDB_INVALIDARG ;
   } else if ( after < 0 )
   {
      setLastErrMsg( "after value must be greater than 0" ) ;
      throw SDB_INVALIDARG ;
   }
   this._after = after ;
   return this ;
}

DiagLog.prototype.before = function( before ) {
   if ( "number" != typeof( before ) )
   {
      setLastErrMsg( "before value must be number" ) ;
      throw SDB_INVALIDARG ;
   } else if ( before < 0 )
   {
      setLastErrMsg( "before value must be greater than 0" ) ;
      throw SDB_INVALIDARG ;
   }
   this._before = before ;
   return this ;
}

DiagLog.prototype.original = function() {
   this._original = true ;
   return this ;
}

DiagLog.prototype.snapshot = function( snapshot ) {
   switch( snapshot )
   {
      case "SNAP_CSCL":
      case "SNAP_SYS":
      case "SNAP_SESSION":
      case "SNAP_QUERY":
      case "SNAP_ALL":
         break ;
      default:
         setLastErrMsg( 'snapshot can only be onr of "SNAP_CSCL", "SNAP_SYS", "SNAP_SESSION", "SNAP_QUERY", "SNAP_ALL"' ) ;
         throw SDB_INVALIDARG ;
   }
   this._snapshot = snapshot ;
   return this ;
}

DiagLog.prototype.core = function() {
   this._core = true ;
   return this ;
}

DiagLog.prototype.trap = function() {
   this._trap = true ;
   return this ;
}

DiagLog.prototype.all = function() {
   this._core = true ;
   this._trap = true ;
   this._snapshot = 'SNAP_ALL' ;
   return this ;
}

DiagLog.prototype.compress = function( compress ) {
   switch( compress )
   {
      case "zip":
      case "tar.gz":
         break ;
      default:
         setLastErrMsg( 'compress value must be "zip" or "tar.gz"' ) ;
         throw SDB_INVALIDARG ;
   }
   this._compress = compress ;
   return this ;
}

DiagLog.prototype._search = function() {
   // close current open result file in DiagLog.next()
   if ( '' != this._logFile ) {
      this._logFile.close() ;
      this._logFile = '' ;
   }

   if ( ! this._needSearch )
   {
      setLastErrMsg( 'No searchable conditions, such as "error()", "keypattern()", "tid()" or "pid()"' ) ;
      throw SDB_INVALIDARG ;
   }

   try
   {
      if ( '' != this._path )
      {
         var regex = new RegExp( '^/.*/diaglog_[0-9]{8}_[0-9]{6}(\.auto)?((?:\.zip)|(?:\.tar.gz)|(?:\/))?$' ) ;
         if ( regex.test( this._path ) )
         {
            // search from collect
            if ( ! File.exist( this._path ) )
            {
               setLastErrMsg( "path " + this._path + " must be exist" ) ;
               throw SDB_FNE ;
            }
            return this._searchFromCollect() ;
         } else
         {
            // search normal diaglog directory
            if ( ! File.exist( this._path ) || ! File.isDir( this._path ) )
            {
               setLastErrMsg( "path " + this._path + " must be a directory" ) ;
               throw SDB_FNE ;
            }
            return this._searchFile() ;
         }
      } else
      {
         return this._searchCluster() ;
      }
   } catch ( e )
   {
      throw e;
   }
}

DiagLog.prototype._checkLocation = function( locationObj, idInfo ) {
   var rc = true ;
   if ( '' != this._locationHostName && rc )
   {
      isChek = true ;
      if ( Array.isArray( this._locationHostName ) )
      {
         var equal = false ;
         for ( var i = 0; i < this._locationHostName.length; i++ )
         {
            if ( this._locationHostName[i] == locationObj.HostName )
            {
               equal = true ;
               break ;
            }
         }
         rc = equal ;
      } else if ( "string" == typeof( this._locationHostName ) )
      {
         rc = this._locationHostName == locationObj.HostName ;
      }
   }

   if ( '' != this._locationServiceName && rc )
   {
      if ( Array.isArray( this._locationServiceName ) )
      {
         var equal = false ;
         for ( var i = 0; i < this._locationServiceName.length; i++ )
         {
            if ( this._locationServiceName[i] == locationObj.ServiceName )
            {
               equal = true ;
               break ;
            }
         }
         rc = equal ;
      } else if ( "string" == typeof( this._locationServiceName ) )
      {
         rc = this._locationServiceName == locationObj.ServiceName ;
      }
   }

   if ( '' != this._locationNodeName && rc )
   {
      if ( Array.isArray( this._locationNodeName ) )
      {
         var equal = false ;
         for ( var i = 0; i < this._locationNodeName.length; i++ )
         {
            var tmpArray = this._locationNodeName[i].split( ':' ) ;
            if ( tmpArray[0] == locationObj.HostName && -1 != tmpArray.indexOf( locationObj.ServiceName ) )
            {
               equal = true ;
               break ;
            }
         }
         rc = equal ;
      } else if ( "string" == typeof( this._locationNodeName ) )
      {
         var tmpArray = this._locationNodeName.split( ':' ) ;
         if ( tmpArray[0] == locationObj.HostName && -1 != tmpArray.indexOf( locationObj.ServiceName ) )
         {
            rc = true ;
         } else
         {
            rc = false ;
         }
      }
   }

   if ( '' != this._locationGroupName && rc )
   {
      if ( Array.isArray( this._locationGroupName ) )
      {
         var equal = false ;
         for ( var i = 0; i < this._locationGroupName.length; i++ )
         {
            if ( this._locationGroupName[i] == locationObj.GroupName )
            {
               equal = true ;
               break ;
            }
         }
         rc = equal ;
      } else if ( "string" == typeof( this._locationGroupName ) )
      {
         rc = this._locationGroupName == locationObj.GroupName ;
      }
   }

   if ( '' != this._locationRole && rc )
   {
      if ( Array.isArray( this._locationRole ) )
      {
         var equal = false ;
         for ( var i = 0; i < this._locationRole.length; i++ )
         {
            if ( "all" == this._locationRole[i] )
            {
               equal = true ;
               break ;
            } else if ( this._locationRole[i] == locationObj.Role )
            {
               equal = true ;
               break ;
            }
         }
         rc = equal ;
      } else if ( "string" == typeof( this._locationRole ) )
      {
         if ( "all" == this._locationRole )
         {
            rc = true ;
         } else
         {
            rc = this._locationRole == locationObj.Role ;
         }
      }
   }

   try
   {
      if ( '' != this._locationNodeID && rc )
      {
         if ( Array.isArray( this._locationNodeID ) )
         {
            var equal = false ;
            for ( var i = 0; i < this._locationNodeID.length; i++ )
            {
               if ( this._locationNodeID[i] == idInfo[locationObj.HostName + ':' + locationObj.ServiceName].NodeID )
               {
                  equal = true ;
                  break ;
               }
            }
            rc = equal ;
         } else if ( "number" == typeof( this._locationNodeID ) )
         {
            rc = this._locationNodeID == idInfo[locationObj.HostName + ':' + locationObj.ServiceName].NodeID ;
         }
      }
   
      if ( '' != this._locationGroupID && rc )
      {
         if ( Array.isArray( this._locationGroupID ) )
         {
            var equal = false ;
            for ( var i = 0; i < this._locationGroupID.length; i++ )
            {
               if ( this._locationGroupID[i] == idInfo[locationObj.GroupName].GroupID )
               {
                  equal = true ;
                  break ;
               }
            }
            rc = equal ;
         } else if ( "number" == typeof( this._locationGroupID ) )
         {
            rc = this._locationGroupID == idInfo[locationObj.GroupName].GroupID ;
         }
      }
   } catch ( e )
   {
      rc = false ;
   }

   return rc ;
}

DiagLog.prototype._searchFromCollect = function() {
   var path = this._path ;
   var pathArray = path.split( '/' ) ;
   var output = '' ;

   try
   {
      var cmd = new Cmd() ;
      var now = new Date( new Date().getTime() + 8 * 60 * 60 * 1000 ) ;
   } catch ( e )
   {
      throw e ;
   }

   if ( '' != this._output )
   {
      output = this._output ;
   } else
   {
      output = this._tmpDir + '/search/cluster_' +  now.toJSON().replace('Z', '').replace( 'T', '-' ) + '.auto' ;
   }

   // remove old dir if more than 10
   try
   {
      var fileArray = cmd.run( 'ls -d ' + this._tmpDir + '/search/cluster_*.auto' ).trimRight( '\n' ).split( '\n' ) ;
      for ( var i = 0; i < fileArray.length - 9; i++ )
      {
         cmd.run( 'rm -rf ' + fileArray[i] ) ;
      }
   } catch ( e ) {}

   // auto generate output dir with current time
   try
   {
      File.mkdir( output, 0777 ) ;
   } catch ( e )
   {
      setLastErrMsg( 'Failed to mkdir "' + output + '", error: ' + getLastErrMsg() ) ;
      throw e ;
   }

   try {
      if ( /.*\.zip$/.test( path ) )
      {
         var rc = cmd.run( 'unzip -v > /dev/null 2>&1;echo $?' ).trimRight('\n');
         if ( '0' != rc )
         {
            setLastErrMsg( 'Cannot extract the archive package, the unzip command is not found' ) ;
            throw SDB_INVALIDARG ;
         } else
         {
            cmd.run( 'unzip -o ' + path + ' -d ' + output ) ;
            path = output + '/' + pathArray[pathArray.length - 1].replace( /\.zip$/, '' ) ;
         }
      } else if  ( /.*\.tar\.gz$/.test( path ) )
      {
         var rc = cmd.run( 'tar --version > /dev/null 2>&1;echo $?' ).trimRight('\n');
         if ( '0' != rc )
         {
            setLastErrMsg( 'Cannot extract the archive package, the tar command is not found' ) ;
            throw SDB_INVALIDARG ;
         } else
         {
            cmd.run( 'tar -xzf ' + path + ' -C ' + output ) ;
            path = output + '/' + pathArray[pathArray.length - 1].replace( /\.tar.gz$/, '' ) ;
         }
      }
   } catch ( e ) {
      setLastErrMsg( 'Failed to unzip "' + path + '", error: ' + getLastErrMsg() ) ;
      throw e ;
   }

   this._nohup = true ;
   var clusterOutput = output + '/diaglog_' ;
   var outputFileArray = [] ;

   try
   {
      var hostArray = cmd.run( 'ls ' + path ).trimRight( '\n' ).split( '\n' ) ;
      for ( var i = 0; i < hostArray.length; i++ )
      {
         var hostName = hostArray[i] ;
         if ( 'analyze' == hostName || 'trap_core_snapshot' == hostName ) { continue ; }
         var nodeFileArray = cmd.run( 'ls ' + path + '/' + hostName ).trimRight( '\n' ).split( '\n' ) ;
         var idInfo = {} ;
         for ( var j = 0; j < nodeFileArray.length; j++ )
         {
            var nodeFile = nodeFileArray[j] ;
            // data_group1_11820_1001_1000
            var nodeInfo = nodeFileArray[j].split( '_' ) ;
            if ( 5 != nodeInfo.length ) {
               setLastErrMsg( 'Failed to parse diaglog in directory ' + path ) ;
               throw SDB_INVALIDARG ;
            }
            var role = nodeInfo[0] ;
            var groupName = nodeInfo[1] ;
            var serviceName = nodeInfo[2] ;
            var nodeID = nodeInfo[3] ;
            var groupID = nodeInfo[4] ;
            var fullFileName = path + "/" + hostName + "/" + nodeFile ;
            var locationObj = { "HostName": hostName, "ServiceName": serviceName, "GroupName": groupName, "Role": role };

            idInfo[groupName] = { "GroupID": groupID } ;
            idInfo[hostName + ":" + serviceName] = { "NodeID": nodeID } ;
            if ( 'standalone' != role && ! this._checkLocation( locationObj, idInfo ) ) { continue ;}
            isSearch = true ;
   
            this._hostName = hostName ;
            this._serviceName = serviceName ;
            this._role = groupName ;
            this._path = fullFileName ;
            this._output = clusterOutput + hostName + "_" + serviceName ;
            this._searchFile( cmd ) ;
            outputFileArray.push( this._output ) ;
         }
      }
   } catch ( e )
   {
      throw e ;
   }

   this._path = path ;
   this._nohup = false ;
   if ( ! isSearch ) {
      return ;
   }

   try
   {
      // waiting for background tasks
      var timeout = 10 * 60 ;
      for ( var i = 0; i < outputFileArray.length; i++ )
      {
         while ( timeout-- )
         {
            rc = parseInt( cmd.run( "test -f " + outputFileArray[i] + "; echo $?" ) ) ;
            if ( ! rc ) {
               break ;
            }
            if ( timeout <= 0 )
            {
               throw SDB_TIMEOUT ;
            } else{
               sleep( 100 ) ;
            }
         }
      }
   } catch ( e )
   {
      setLastErrMsg( 'Failed to wait for searching results, error: ' + getLastErrMsg() );
      throw e ;
   }

   // merge result
   this._output = clusterOutput + 'result' ;
   this._outputFile = this._output ;

   try
   {
      var resultDir = this._output.substring( 0, this._output.lastIndexOf( '/' ) );
      if ( '' != resultDir )
      {
         File.mkdir( resultDir, 0777 ) ;
      }
      var resultFile = new File( this._output, 0666, SDB_FILE_READWRITE | SDB_FILE_CREATE ) ;
      resultFile.truncate() ;
      var content = "";
      for ( var i = 0; i < outputFileArray.length; i++ )
      {
         try
         {
            var localFile = new File( outputFileArray[i], 0666, SDB_FILE_READONLY ) ;
            while( true )
            {
               content = localFile.readContent( 4 * 1024 * 1024 ) ;
               resultFile.writeContent( content ) ;
               content.clear() ;
            }
         } catch ( e )
         {
            if ( -9 != e )
            {
               throw e ;
            }
         } finally
         {
            if ( null != localFile )
            {
               localFile.close() ;
            }
         }
      }
   } catch ( e )
   {
      setLastErrMsg( 'Failed to merge search result, error: ' + getLastErrMsg() );
      throw e ;
   } finally
   {
      if ( null != resultFile )
      {
         resultFile.close() ;
      }
   }

   // sort result
   var shellCmd = this._logTool + ' -o "' + this._output + '" --sort-only' ;
   if ( '' != this._original )
   {
      shellCmd += ' -O' ;
   }
   if ( 0 < this._limit )
   {
      shellCmd += ' -n ' + this._limit ;
   }

   try {
      cmd.run( shellCmd ) ;
   } catch ( e ) {
      setLastErrMsg( 'Failed to sort result with "' + shellCmd + '", error: ' + getLastErrMsg() );
      throw e ;
   }

   this._loopFile = true ;
   return this._output ;
}

DiagLog.prototype._searchCluster = function() {
   var rc = "" ;
   try
   {
      if ( '' != this.cipherUser)
      {
         var db = new Sdb( this.hostname, this.svcname, this.cipherUser ) ;
      } else
      {
         var db = new Sdb( this.hostname, this.svcname, this.user, this.password ) ;
      }
   } catch ( e )
   {
      throw e ;
   }

   this._nohup = true ;
   var outputFileArray = [] ;

   try
   {
      var now = new Date( new Date().getTime() + 8 * 60 * 60 * 1000 );
      var oma = new Oma() ;
      var localCmd = new Cmd() ;
   } catch ( e )
   {
      throw e;  
   }

   // remove old dir if more than 10
   try
   {
      var fileArray = localCmd.run( 'ls -d ' + this._tmpDir + '/search/cluster_*.auto 2>/dev/null' ).trimRight( '\n' ).split( '\n' ) ;
      for ( var i = 0; i < fileArray.length - 9; i++ )
      {
         localCmd.run( 'rm -rf ' + fileArray[i] ) ;
      }
   } catch ( e ) {}

   // auto generate output dir with current time
   try
   {
      File.mkdir( this._tmpDir + '/search/cluster_' + now.toJSON().replace('Z', '').replace( 'T', '-' ) + '.auto/', 0777 ) ;
   } catch ( e )
   {
      setLastErrMsg( 'Failed to mkdir "' + this._tmpDir + '/search/cluster_' + now.toJSON().replace('Z', '').replace( 'T', '-' ) + '.auto/", error: ' + getLastErrMsg() );
      throw e;  
   }

   var output = this._output ;
   var clusterOutput = this._tmpDir + '/search/cluster_' + now.toJSON().replace('Z', '').replace( 'T', '-' ) + '.auto/diaglog_'  ;
   var cursor ;
   var idInfo = {} ;
   var isSearch = false ;
   var logTool = this._logTool ;

   // skip standalone nodes
   try {
      var isStandalone = db.exec('select role from $SNAPSHOT_CONFIGS limit 1').current().toObj().role ;
   } catch ( e ) {
      setLastErrMsg( 'Failed to get info from "select role from $SNAPSHOT_CONFIGS", error: ' + getLastErrMsg() ) ;
      throw e ;
   }

   if ( ( '' != this._locationNodeID || '' != this._locationGroupID ) && 'standalone' != isStandalone )
   {
      try
      {
         cursor = db.exec('select Group,GroupID,GroupName from $LIST_GROUP split by Group') ;
         while ( cursor.next() )
         {
            var current = cursor.current().toObj() ;
            idInfo[current.GroupName] = { "GroupID": current.GroupID } ;
            idInfo[current.Group.HostName + ":" + current.Group.Service[0].Name] = { "NodeID": current.Group.NodeID } ;
         }
      } catch ( e )
      {
         setLastErrMsg( 'Failed get node info from "select Group,GroupID,GroupName from $LIST_GROUP split by Group"' ) ;
         throw e ;
      } finally
      {
         if ( null != cursor )
         {
            cursor.close() ;
         }
      }
   }

   var diagpathObj = {} ;
   try {
      cursor = db.exec('select NodeName,diagpath from $SNAPSHOT_CONFIGS');
      while ( cursor.next() )
      {
         var current = cursor.current().toObj() ;
         diagpathObj[current.NodeName] = current.diagpath;
      }
      cursor.close() ;
   } catch ( e )
   {
      setLastErrMsg( 'Failed get node info from "select NodeName,diagpath from $SNAPSHOT_CONFIGS"' ) ;
      throw e ;
   }

   try {
      cursor = db.exec('select HostName, push(ServiceName) as ServiceName, push(GroupName) as GroupName from $SNAPSHOT_SYSTEM group by HostName') ;
   } catch ( e )
   {
      setLastErrMsg( 'Failed get node info from "select HostName, push(ServiceName) as ServiceName, push(GroupName) as GroupName from $SNAPSHOT_SYSTEM group by HostName"' ) ;
      throw e ;
   }

   try
   {
      while ( cursor.next() )
      {
         var current = cursor.current().toObj() ;
         var HostName = current.HostName ;
         var ServiceNameArray = current.ServiceName ;
         var omaSvcName = oma.getAOmaSvcName( HostName ) ;
         var remote = new Remote( HostName, omaSvcName ) ;
         var cmd = remote.getCmd() ;
         this._logTool = remote.getSystem().getEWD() + '/../tools/diaglog/logSearchTool.sh' ;
         try
         {
            // remove old dir if more than 10 on remote
            var dirArray = cmd.run( "ls -d " + this._tmpDir + "/search/cluster_*.auto 2>/dev/null" ).trimRight( '\n' ).split( "\n" ) ;
            for ( var i = 0; i < dirArray.length - 10; i++ )
            {
               cmd.run( "rm -rf " + dirArray[i] ) ;
            }
         } catch ( e ) {}
         for (var i = 0; i < ServiceNameArray.length; i++)
         {
            var locationObj = { "HostName": HostName, "ServiceName": ServiceNameArray[i], "GroupName": current.GroupName[i], "Role": "" };
            if ( 'SYSCoord' == current.GroupName[i] )
            {
               locationObj["Role"] = 'coord' ;
            } else if ( 'SYSCatalogGroup' == current.GroupName[i] )
            {
               locationObj["Role"] = 'catalog' ;
            } else if ( '' != current.GroupName[i] )
            {
               locationObj["Role"] = 'data' ;
            } else
            {
               locationObj["Role"] = 'standalone' ;
            }
            if ( 'standalone' != isStandalone && ! this._checkLocation( locationObj, idInfo ) ) { continue ;}
            isSearch = true ;

            this._hostName = HostName ;
            this._serviceName = ServiceNameArray[i] ;
            if ( '' != current.GroupName[i] )
            {
               this._role = current.GroupName[i] ;
            } else
            {
               this._role = 'standalone' ;
               locationObj["GroupName"] = 'standalone'
            }
            this._path = diagpathObj[HostName + ":" + ServiceNameArray[i]];
            this._output = clusterOutput + HostName + "_" + ServiceNameArray[i] ;
            this._searchFile( cmd ) ;
            outputFileArray.push( { "HostName": HostName, "output": this._output, "remote": remote, "cmd": cmd } ) ;
         }
      }
   } catch ( e )
   {
      throw e ;
   } 

   this._nohup = false ;
   if ( ! isSearch ) {
      return ;
   }

   try
   {
      // waiting for background tasks
      var timeout = 100 * 60 ;
      for ( var i = 0; i < outputFileArray.length; i++ )
      {
         while ( timeout-- )
         {
            rc = parseInt( outputFileArray[i].cmd.run( "test -f " + outputFileArray[i].output + "; echo $?" ) ) ;
            if ( ! rc ) {
               break ;
            }
            if ( timeout <= 0 )
            {
               throw SDB_TIMEOUT ;
            } else{
               sleep( 100 ) ;
            }
         }
      }
   } catch ( e )
   {
      setLastErrMsg( 'Failed to wait for searching results, error: ' + getLastErrMsg() );
      throw e ;
   } finally
   {
      if ( null != cursor )
      {
         cursor.close() ;
      }
      if ( null != db )
      {
         db.close() ;
      }
   }

   // merge result
   if ( '' != output )
   {
      this._output = output ;
   } else
   {
      this._output = clusterOutput + 'result' ;
   }
   this._outputFile = this._output ;

   try
   {
      var resultDir = this._output.substring( 0, this._output.lastIndexOf( '/' ) );
      if ( '' != resultDir )
      {
         File.mkdir( resultDir, 0777 ) ;
      }
      var resultFile = new File( this._output, 0666, SDB_FILE_READWRITE | SDB_FILE_CREATE ) ;
      resultFile.truncate() ;
      var content = "";
      for ( var i = 0; i < outputFileArray.length; i++ )
      {
         var remoteFile = outputFileArray[i].remote.getFile( outputFileArray[i].output ) ;
         var localFile = new File( outputFileArray[i].output, 0666, SDB_FILE_WRITEONLY | SDB_FILE_CREATE ) ;
         try
         {
            while( true )
            {
               content = remoteFile.readContent( 4 * 1024 * 1024 ) ;
               resultFile.writeContent( content ) ;
               localFile.writeContent( content ) ;
               content.clear() ;
            }
         } catch ( e )
         {
            if ( -9 != e )
            {
               throw e ;
            }
         } finally
         {
            if ( null != localFile )
            {
               localFile.close() ;
            }
            if ( null != remoteFile )
            {
               remoteFile.close() ;
            }
         }
      }
   } catch ( e )
   {
      setLastErrMsg( 'Failed to merge search result, error: ' + getLastErrMsg() ) ;
      throw e ;
   } finally
   {
      if ( null != resultFile )
      {
         resultFile.close() ;
      }
   }

   // sort result
   this._logTool = logTool ;
   var shellCmd = this._logTool + ' -o "' + this._output + '" --sort-only' ;
   if ( '' != this._original )
   {
      shellCmd += ' -O' ;
   }
   if ( 0 < this._limit )
   {
      shellCmd += ' -n ' + this._limit ;
   }

   try {
      localCmd.run( shellCmd ) ;
   } catch ( e ) {
      setLastErrMsg( 'Failed to sort result with "' + shellCmd + '", error: ' + getLastErrMsg() ) ;
      throw e ;
   }

   for ( var i = 0; i < outputFileArray.length; i++ )
   {
      outputFileArray[i].remote.close() ;
   }

   this._loopFile = true ;
   return this._output ;
}

DiagLog.prototype._searchFile = function( cmd ) {
   try
   {
      if ( undefined == cmd ) {
         var cmd = new Cmd() ;
      }
   } catch ( e )
   {
      throw e ;
   }

   var shellCmd = this._logTool + ' -d "' + this._path + '"' ;

   if ( '' != this._lastFile )
   {
      shellCmd += ' -f ' + this._lastFile ;
   }

   if ( '' != this._lastest )
   {
      shellCmd += ' -r "' + this._lastest + '"' ;
   }

   if ( '' != this._timeBegin )
   {
      shellCmd += ' -s "' + this._timeBegin + '"' ;
   }

   if ( '' != this._timeEnd )
   {
      shellCmd += ' -e "' + this._timeEnd + '"' ;
   }

   if ( '' != this._output )
   {
      shellCmd += ' -o "' + this._output + '"' ;
      this._outputFile = this._output ;
   } else
   {
      // auto generate output dir with current time
      var now = new Date( new Date().getTime() + 8 * 60 * 60 * 1000 );
      this._outputFile = this._tmpDir + '/search/log_' + now.toJSON().replace('Z', '').replace( 'T', '-' ) + '.auto' ;
      shellCmd += ' -o "' + this._outputFile + '"' ;
      // remove old dir if more than 10
      try
      {
         var fileArray = cmd.run( "ls " + this._tmpDir + "/search/log_*.auto 2>/dev/null" ).trimRight( '\n' ).split( "\n" ) ;
         for ( var i = 0; i < fileArray.length - 9; i++ )
         {
            cmd.run( "rm -rf " + fileArray[i] ) ;
         }
      } catch ( e ) {}
   }

   if ( '' != this._diaglevel )
   {
      shellCmd += ' -l ' + this._diaglevel ;
   }

   if ( '' != this._error )
   {
      shellCmd += ' -E "' + this._error + '"' ;
   }

   if ( '' != this._keypattern )
   {
      shellCmd += ' -m "' + this._keypattern + '"' ;
   }

   if ( '' != this._pid )
   {
      shellCmd += ' -p ' + this._pid ;
   }

   if ( '' != this._tid )
   {
      shellCmd += ' -t ' + this._tid ;
   }

   if ( this._original )
   {
      shellCmd += ' -O' ;
   }

   if ( '' != this._after )
   {
      shellCmd += ' -a ' + this._after ;
   }

   if ( '' != this._before )
   {
      shellCmd += ' -b ' + this._before ;
   }

   if ( 0 < this._limit )
   {
      shellCmd += ' -n ' + this._limit ;
   }

   if ( '' != this._role )
   {
      shellCmd += ' -R "' + this._role + '"' ;
   }

   if ( '' != this._serviceName )
   {
      shellCmd += ' -S "' + this._serviceName + '"' ;
   }

   if ( '' != this._hostName ) {
      shellCmd += ' -H "' + this._hostName + '"' ;
   }

   if ( this._fileOnly )
   {
      shellCmd += ' --files-only' ;
   }

   try
   {
      if ( "" != shellCmd )
      {
         if ( this._nohup )
         {
            cmd.start( shellCmd ) ;
         } else
         {
            cmd.run( shellCmd ) ;
            this._loopFile = true ;
         }
      }
   } catch ( e )
   {
      setLastErrMsg( 'Failed to search log with cmd "' + shellCmd + '", error: "' + getLastErrMsg() + '"' ) ;
      throw e ;
   }
   return this._outputFile ;
}

DiagLog.prototype.next = function( num ) {
   if ( null == num )
   {
      num = 1 ;
   }
   if ( "number" != typeof( num ) )
   {
      setLastErrMsg( "next value must be number" ) ;
      throw SDB_INVALIDARG ;
   } else if ( num <= 0 )
   {
      setLastErrMsg( "next value must be greater than 0" ) ;
      throw SDB_INVALIDARG ;
   }

   if ( ! this._loopFile )
   {
      return ;
   }

   try
   {
      if ( ! File.exist( this._outputFile ) || ! File.isFile( this._outputFile ) )
      {
         this._loopFile = false ;
         return ;
      }

      if ( this._readFile != this._outputFile && '' != this._logFile ) {
         this._logFile.close() ;
         this._logFile = '' ;
      }
   
      if ( '' == this._logFile )
      {
         this._logFile = new File( this._outputFile, 0644, SDB_FILE_READONLY ) ;
         this._readFile = this._outputFile ;
      }
   } catch ( e )
   {
      setLastErrMsg( 'Failed to open file ' + this._outputFile + ', error: ' + getLastErrMsg() );
      throw e ;   
   }

   var content ;
   var result = '' ;
   var preNodeName = '' ;
   try
   {
      var i = 0 ;
      var isEmptyLine = false ;
      var isNodeNameLine = false ;
      var curNodeName = '' ;
      var regex = new RegExp( '^={2,}[^=]+={2,}$' );
      while( content = this._logFile.readLine() )
      {
         if ( regex.test( content.trimRight( '\n' ) ) )
         {
            isNodeNameLine = true ;
            curNodeName = content ;
         }

         if ( 0 == content.trimRight( '\n' ).length )
         {
            isEmptyLine = true ;
         }

         if ( ! this._original )
         {
            result += content ;
            i++ ;
         } else
         {
            if ( isNodeNameLine )
            {
               isNodeNameLine = false ;
               if ( curNodeName != preNodeName || '' == preNodeName )
               {
                  preNodeName = curNodeName ;
                  result += curNodeName ;
               }
            } else
            {
               result += content
            }

            if ( isEmptyLine )
            {
               i++ ;
               isEmptyLine = false ;
            }
         }

         if ( i >= num )
         {
            break ;
         } 
      }
   } catch ( e )
   {
      this._logFile.close() ;
      this._logFile = '' ;
      this._loopFile = false ;
      if ( SDB_EOF != e )
      {
         setLastErrMsg( 'Failed to read next line from ' + this._outputFile + ', error: ' + getLastErrMsg() );
         throw e ;
      }
   }
   return result ;
}

DiagLog.prototype._scp = function( src, dst ) {
   if( "string" != typeof( src ) )
   {
      setLastErrMsg( "src must be string" ) ;
      throw SDB_INVALIDARG ;
   }
   if( "string" != typeof( dst ) )
   {
      setLastErrMsg( "dst must be string" ) ;
      throw SDB_INVALIDARG ;
   }

   var srcFile ;
   var dstFile ;
   var COPY_UNIT = 4*1024*1024 ;
   var srcArr = src.split( "@" ) ;
   var dstFilename ;
   var mode ;
   if( srcArr.length > 1 )
   {
      var hostPortSplit = srcArr[0].split( ":" ) ;
      var remote = new Remote( hostPortSplit[0], hostPortSplit[1] ) ;
      var fileMgr = remote.getFile() ;

      if( false == fileMgr.exist( srcArr[1] ) )
      {
         setLastErrMsg( "src not exist" ) ;
         throw SDB_FNE ;
      }
      mode = fileMgr._getPermission( srcArr[1] ) ;
      srcFile = remote.getFile( srcArr[1], 0644, SDB_FILE_READONLY ) ;
   }
   else
   {
      if( false == File.exist( srcArr[0] ) )
      {
         setLastErrMsg( "src not exist" ) ;
         throw SDB_FNE ;
      }
      mode = File._getPermission( srcArr[0] ) ;
      srcFile = new File( srcArr[0], 0666, SDB_FILE_READONLY ) ;
   }

   var dstArr = dst.split( "@" ) ;
   if( dstArr.length > 1 )
   {
      var hostPortSplit = dstArr[0].split( ":" ) ;
      var remote = new Remote( hostPortSplit[0], hostPortSplit[1] ) ;
      var fileMgr = remote.getFile() ;
      dstFilename = dstArr[1] ;
      if( true == fileMgr.exist( dstFilename ) )
      {
         setLastErrMsg( "dst file " + dst + " exist" ) ;
         throw SDB_FE ;
      }
      else
      {
         dstFile = remote.getFile( dstFilename, mode,
                                   SDB_FILE_CREATEONLY | SDB_FILE_READWRITE ) ;
      }
   }
   else
   {
      dstFilename = dstArr[0] ;
      if( true == File.exist( dstFilename ) )
      {
         setLastErrMsg( "dst file " + dst + " exist" ) ;
         throw SDB_FE ;
      }
      else
      {
         dstFile = new File( dstFilename, mode,
                             SDB_FILE_CREATEONLY | SDB_FILE_READWRITE ) ;
      }
   }

   try
   {
      while( true )
      {
         var fileContent = srcFile.readContent( COPY_UNIT ) ;
         dstFile.writeContent( fileContent ) ;
         fileContent.clear() ;
      }
   }
   catch( e )
   {
      srcFile.close() ;
      dstFile.close() ;
      var errValue = e.message || e;
      if( -9 != errValue )
      {
         throw e ;
      }
   }
}

DiagLog.prototype._collectTrapAndCore = function ( db, collectOutput ) {
   try
   {
      var dstDir = collectOutput + '/trap_core_snapshot' ;
      File.mkdir( dstDir, 0777 ) ;
   } catch ( e )
   {
      setLastErrMsg( 'Failed to mkdir ' + dstDir + ', error: ' + getLastErrMsg() );
      throw e ;
   }

   var diagpathObj = {} ;
   try {
      var cursor = db.exec('select NodeName,diagpath from $SNAPSHOT_CONFIGS');
      while ( cursor.next() )
      {
         var current = cursor.current().toObj() ;
         diagpathObj[current.NodeName] = current.diagpath;
      }
      cursor.close() ;
   } catch ( e )
   {
      setLastErrMsg( 'Failed get node info from "select NodeName,diagpath from $SNAPSHOT_CONFIGS"' ) ;
      throw e ;
   }

   try
   {
      var oma = new Oma() ;
      var cursor = db.exec('select HostName, push(ServiceName) as ServiceName, push(GroupName) as GroupName from $SNAPSHOT_SYSTEM group by HostName') ;
      while ( cursor.next() )
      {
         var current = cursor.current().toObj() ;
         var omaSvcName = oma.getAOmaSvcName( current.HostName ) ;
         var remote = new Remote( current.HostName, omaSvcName ) ;
         var cmd = remote.getCmd() ;
         for ( var i = 0; i < current.ServiceName.length; i++ )
         {
            var coreArray = [] ;
            var trapArray = [] ;
            var GroupName = current.GroupName[i] ;
            if ( '' == GroupName )
            {
               GroupName = 'standalone' ;
            }
            if ( this._core )
            {
               try
               {
                  coreArray = cmd.run( 'ls ' + diagpathObj[current.HostName + ":" + current.ServiceName[i]] + '/*.core 2>/dev/null' ).trimRight( '\n' ).split( '\n' ) ;
               } catch ( e )
               {
                  if ( 2 != e )
                  {
                     setLastErrMsg( 'Failed to get the core file of the cluster, error: ' + getLastErrMsg() );
                     throw e ;
                  }
               }
            }
            if ( this._trap )
            {
               try
               {
                  trapArray = cmd.run( 'ls ' + diagpathObj[current.HostName + ":" + current.ServiceName[i]] + '/*.trap 2>/dev/null' ).trimRight( '\n' ).split( '\n' ) ;
               } catch ( e )
               {
                  if ( 2 != e )
                  {
                     setLastErrMsg( 'Failed to get the trap file of the cluster, error: ' + getLastErrMsg() );
                     throw e ;
                  }
               }
            }
            var collectArray = coreArray.concat(trapArray) ;
            var srcFile = '' ;
            var dstFile = '' ;
            for ( var j = 0; j < collectArray.length; j++ )
            {
               if ( '' == collectArray[j] ) { continue ; }
               var collectNameArray = collectArray[j].split( '/' ) ;
               srcFile = current.HostName + ':' + omaSvcName + '@' + collectArray[j] ;
               dstFile = dstDir + '/' + current.HostName + '_' + current.ServiceName[i] + '_' + GroupName + '_' + collectNameArray[ collectNameArray.length - 1 ] ;
               this._scp( srcFile, dstFile ) ;
            }
         }
      }
   } catch ( e )
   {
      setLastErrMsg( 'Failed to collect core or trap, error: ' + getLastErrMsg() );
      throw e ;
   } finally
   {
      if ( null != cursor )
      {
         cursor.close() ;
      }
   }
}

DiagLog.prototype._getSnapshot = function ( db, snapshot, filename, ignore, history ) {
   var cursor ;
   try
   {
      cursor = db.exec( 'select * from ' + snapshot ) ;
   } catch ( e )
   {
      // ignore standalone nodes
      if ( -159 == e )
      {}
      // some snapshots may not exist in earlier versions
      else if ( -6 == e && ignore )
      {} else
      {
         setLastErrMsg( 'Failed to select * from ' + snapshot + ', error: ' + getLastErrMsg() );
         throw e ;
      }
   }

   try
   {
      var file = new File( filename, 0644, SDB_FILE_READWRITE | SDB_FILE_CREATE ) ;
   } catch ( e )
   {
      if ( null != cursor )
      {
         cursor.close() ;
      }
      setLastErrMsg( 'Failed to open ' + filename + ', error: ' + getLastErrMsg() );
      throw e ;
   }

   try
   {
      if ( null != cursor )
      {
         while ( cursor.next() )
         {
            file.write( JSON.stringify( cursor.current().toObj(), null, 2 ) + '\n' ) ;
         }
      }
   } catch ( e )
   {
      setLastErrMsg( 'Failed to write ' + snapshot + ' to ' + filename + ', error: ' + getLastErrMsg() );
      throw e ;
   } finally
   {
      if ( null != cursor )
      {
         cursor.close() ;
      }
      if ( null != file )
      {
         file.close() ;
      }
   }

   if ( history )
   {
      try
      {
         cursor = db.exec( 'select * from ' + snapshot + ' /*+use_option(viewHistory,true)*/' ) ;
      } catch ( e )
      {
         // ignore standalone nodes
         if ( -159 == e )
         {}
         // some snapshots may not exist in earlier versions
         else if ( -6 == e && ignore )
         {} else
         {
            setLastErrMsg( 'Failed to select * from ' + snapshot + ', error: ' + getLastErrMsg() );
            throw e ;
         }
      }

      try
      {
         filename += '_history' ;
         var file = new File( filename , 0644, SDB_FILE_READWRITE | SDB_FILE_CREATE ) ;
      } catch ( e )
      {
         if ( null != cursor )
         {
            cursor.close() ;
         }
         setLastErrMsg( 'Failed to open ' + filename + ', error: ' + getLastErrMsg() );
         throw e ;
      }

      try
      {
         if ( null != cursor )
         {
            while ( cursor.next() )
            {
               file.write( JSON.stringify( cursor.current().toObj(), null, 2 ) + '\n' ) ;
            }
         }
      } catch ( e )
      {
         setLastErrMsg( 'Failed to write history ' + snapshot + ' to ' + filename + ', error: ' + getLastErrMsg() );
         throw e ;
      } finally
      {
         if ( null != cursor )
         {
            cursor.close() ;
         }
         if ( null != file )
         {
            file.close() ;
         }
      }
   }
}

DiagLog.prototype._collectSnapshot = function ( db, collectOutput ) {
   try
   {
      var dirName = collectOutput + '/trap_core_snapshot' ;
      File.mkdir( dirName, 0777 ) ;
      if ( 'SNAP_CSCL' == this._snapshot || 'SNAP_ALL' == this._snapshot )
      {
         this._getSnapshot( db, '$SNAPSHOT_CS', dirName + '/snapshot_cs', false, false );
         this._getSnapshot( db, '$SNAPSHOT_CL', dirName + '/snapshot_cl', false, false );
         this._getSnapshot( db, '$SNAPSHOT_CATA', dirName + '/snapshot_cata', false, false );
      }
      if ( 'SNAP_SYS' == this._snapshot || 'SNAP_ALL' == this._snapshot )
      {
         this._getSnapshot( db, '$SNAPSHOT_SYSTEM', dirName + '/snapshot_system', false, false );
         this._getSnapshot( db, '$SNAPSHOT_CONFIGS', dirName + '/snapshot_configs', false, false );
         this._getSnapshot( db, '$SNAPSHOT_DB', dirName + '/snapshot_db', false, false );
         this._getSnapshot( db, '$SNAPSHOT_HEALTH', dirName + '/snapshot_health', false, false );
         this._getSnapshot( db, '$SNAPSHOT_SEQUENCES', dirName + '/snapshot_sequences', false, false );
         this._getSnapshot( db, '$SNAPSHOT_SVCTASKS', dirName + '/snapshot_svctasks', true, false );
         this._getSnapshot( db, '$SNAPSHOT_TASKS', dirName + '/snapshot_tasks', true, false );
      }
      if ( 'SNAP_SESSION' == this._snapshot || 'SNAP_ALL' == this._snapshot )
      {
         this._getSnapshot( db, '$SNAPSHOT_SESSION', dirName + '/snapshot_session', false, false );
         this._getSnapshot( db, '$SNAPSHOT_CONTEXT', dirName + '/snapshot_context', false, false );
      }
      if ( 'SNAP_QUERY' == this._snapshot || 'SNAP_ALL' == this._snapshot )
      {
         // always collect history, even if it may be empty
         this._getSnapshot( db, '$SNAPSHOT_QUERIES', dirName + '/snapshot_queries', true, true );
         this._getSnapshot( db, '$SNAPSHOT_LOCKWAITS', dirName + '/snapshot_lockwaits', true, true );
         this._getSnapshot( db, '$SNAPSHOT_LATCHWAITS', dirName + '/snapshot_latchwaits', true, false );
         this._getSnapshot( db, '$SNAPSHOT_TRANS', dirName + '/snapshot_trans', false, false );
         this._getSnapshot( db, '$SNAPSHOT_ACCESSPLANS', dirName + '/snapshot_accessplans', false, false );
         this._getSnapshot( db, '$SNAPSHOT_INDEXSTATS', dirName + '/snapshot_indexstats', true, false );
         this._getSnapshot( db, '$SNAPSHOT_TRANSDEADLOCK', dirName + '/snapshot_transdeadlock', true, false );
         this._getSnapshot( db, '$SNAPSHOT_TRANSWAIT', dirName + '/snapshot_transwait', true, false );
      }
   } catch ( e )
   {
      throw e;
   }
}

DiagLog.prototype._collect = function() {
   var db ;
   try
   {
      if ( '' != this.cipherUser)
      {
         db = new Sdb( this.hostname, this.svcname, this.cipherUser ) ;
      } else
      {
         db = new Sdb( this.hostname, this.svcname, this.user, this.password ) ;
      }
      var cmd = new Cmd() ;
      var oma = new Oma() ;
      var now = new Date() ;
   } catch ( e )
   {
      throw e ;
   }

   // check zip or tar command
   try {
      if ( 'zip' == this._compress )
      {
         var rc = cmd.run( 'zip --version > /dev/null 2>&1;echo $?' ).trimRight('\n');
         if ( '0' != rc )
         {
            setLastErrMsg( 'Cannot create the archive package, the zip command is not found' ) ;
            throw SDB_INVALIDARG ;
         }
      } else
      {
         var rc = cmd.run( 'tar --version > /dev/null 2>&1;echo $?' ).trimRight('\n');
         if ( '0' != rc )
         {
            setLastErrMsg( 'Cannot create the archive package, the tar command is not found' ) ;
            throw SDB_INVALIDARG ;
         }
      }
   } catch ( e ) {
      throw e ;
   }

   // auto generate output dir with current time
   var collectOutput = '' ;
   var collectOutputDir = 'diaglog_' ;
   var autoGenerate = '' ;
   this._output = '' ;
   if ( '' != this._path )
   {
      try
      {
         if ( File.exist( this._path ) )
         {
            var isDir = File.isDir( this._path );
            if ( !isDir )
            {
               setLastErrMsg( 'In the collect(), the value of the path() must be a directory' ) ;
               throw SDB_INVALIDARG ;
            }
         }

      } catch ( e ) {
         throw e ;
      }
      collectOutput += this._path ;
      this._path = '' ;
   } else
   {
      collectOutput += this._tmpDir + '/collect' ;
      autoGenerate = '.auto' ;
      // remove old dir if more than 10
      try
      {
         var fileArray = cmd.run( 'ls -d ' + this._tmpDir + '/collect/diaglog_*.auto 2>/dev/null' ).trimRight( '\n' ).split( '\n' ) ;
         for ( var i = 0; i < fileArray.length - 9; i++ )
         {
            cmd.run( "rm -rf " + fileArray[i] ) ;
         }
      } catch ( e ) {}
      try
      {
         var fileArray = cmd.run( 'ls ' + this._tmpDir + '/collect/diaglog_*.auto.zip 2>/dev/null' ).trimRight( '\n' ).split( '\n' ) ;
         for ( var i = 0; i < fileArray.length - 9; i++ )
         {
            cmd.run( "rm -rf " + fileArray[i] ) ;
         }
      } catch ( e ) {}
      try
      {
         var fileArray = cmd.run( 'ls ' + this._tmpDir + '/collect/diaglog_*.auto.tar.gz 2>/dev/null' ).trimRight( '\n' ).split( '\n' ) ;
         for ( var i = 0; i < fileArray.length - 9; i++ )
         {
            cmd.run( "rm -rf " + fileArray[i] ) ;
         }
      } catch ( e ) {}
   }
   collectOutputDir += now.getFullYear() ;
   if ( now.getMonth() >= 9 )
   {
      collectOutputDir += (now.getMonth() + 1) ;
   } else
   {
      collectOutputDir += '0' + (now.getMonth() + 1) ;
   }
   if ( now.getDate() > 9 )
   {
      collectOutputDir += (now.getDate()) + "_" ;
   } else
   {
      collectOutputDir += '0' + (now.getDate()) + "_" ;
   }
   if ( now.getHours() > 9 )
   {
      collectOutputDir += (now.getHours()) ;
   } else
   {
      collectOutputDir += '0' + (now.getHours()) ;
   }
   if ( now.getMinutes() > 9 )
   {
      collectOutputDir += (now.getMinutes()) ;
   } else
   {
      collectOutputDir += '0' + (now.getMinutes()) ;
   }
   if ( now.getSeconds() > 9 )
   {
      collectOutputDir += (now.getSeconds()) ;
   } else
   {
      collectOutputDir += '0' + (now.getSeconds()) ;
   }
   collectOutputDir += autoGenerate ;
   collectOutput += '/' + collectOutputDir ;

   var needCompress = false ;
   if ( this._needSearch )
   {
      this._original = false ;
      this._path = '' ;
      // use --files-only to search log files name
      this._fileOnly = true ;
      try
      {
         this._search() ;
      } catch ( e )
      {
         throw e ;
      }
      this._fileOnly = false ;

      var nodeInfoObj = {} ;
      var content ;
      var cursor ;
      try
      {
         // skip standalone nodes
         try {
            var isStandalone = db.exec('select role from $SNAPSHOT_CONFIGS limit 1').current().toObj().role ;
         } catch ( e ) {
            setLastErrMsg( 'Failed to get info from "select role from $SNAPSHOT_CONFIGS", error: ' + getLastErrMsg() ) ;
            throw e ;
         }
         if ( 'standalone' != isStandalone ) {
            try
            {
               cursor = db.exec('select Group,GroupID,GroupName from $LIST_GROUP split by Group') ;
               while ( cursor.next() )
               {
                  var current = cursor.current().toObj() ;
                  var info = {} ;
                  info['NodeID'] = current.Group.NodeID ;
                  info['GroupID'] = current.GroupID ;
                  info['GroupName'] = current.GroupName ;
                  if ( 'SYSCoord' == current.GroupName )
                  {
                     info["Role"] = 'coord' ;
                  } else if ( 'SYSCatalogGroup' == current.GroupName )
                  {
                     info["Role"] = 'catalog' ;
                  } else
                  {
                     info["Role"] = 'data' ;
                  }
                  nodeInfoObj[current.Group.HostName + ':' + current.Group.Service[0].Name] = info ;
               }
            } catch ( e )
            {
               setLastErrMsg( 'Failed to get info from "select Group,GroupID,GroupName from $LIST_GROUP split by Group", error: ' + getLastErrMsg() );
               throw e ;
            }
         } else {
            try
            {
               cursor = db.exec('select HostName,ServiceName,GroupName from $SNAPSHOT_DB') ;
               while ( cursor.next() )
               {
                  var current = cursor.current().toObj() ;
                  var info = {} ;
                  info['NodeID'] = 0 ;
                  info['GroupID'] = 0 ;
                  info['GroupName'] = 'standalone' ;
                  info["Role"] = 'standalone' ;
                  nodeInfoObj[current.HostName + ':' + current.ServiceName] = info ;
               }
            } catch ( e )
            {
               setLastErrMsg( 'Failed to get info from "select HostName,ServiceName,GroupName from $SNAPSHOT_DB", error: ' + getLastErrMsg() );
               throw e ;
            }
         }
      } catch ( e )
      {
         throw e ;
      } finally
      {
         if ( null != cursor )
         {
            cursor.close() ;
         }
      }

      try
      {
         while ( content = this.next( 1 ) )
         {
            content = content.trim( '\n' );
            if ( undefined == content || '' == content ) { continue ; }
            var hostName = content.split( '@@' )[0] ;
            var serviceName = content.split( '@@' )[1] ;
            var fileName = content.split( '@@' )[2] ;
            var nodeName = hostName + ':' + serviceName ;
            var omaSvcName = oma.getAOmaSvcName( hostName ) ;
            var filenameArray = fileName.split( '/' );
            var srcFile = hostName + ':' + omaSvcName + '@' + fileName ;
            var role = nodeInfoObj[nodeName].Role ;
            var groupName = nodeInfoObj[nodeName].GroupName ;
            var nodeID = nodeInfoObj[nodeName].NodeID ;
            var groupID = nodeInfoObj[nodeName].GroupID ;
            var dstDir = collectOutput + '/' + hostName + '/' + role + '_' + groupName + '_' + serviceName + '_' + nodeID + '_' + groupID ;
            var dstFile = dstDir + '/' + filenameArray[filenameArray.length - 1] ;
            File.mkdir( dstDir, 0777 ) ;
            this._scp( srcFile, dstFile ) ;
            needCompress = true ;
         }
      } catch ( e )
      {
         setLastErrMsg( 'Failed to get the collected log target, error: ' + getLastErrMsg() );
         throw e ;
      }
   }

   try
      {
      if ( this._trap || this._core )
      {
         needCompress = true ;
         this._collectTrapAndCore( db, collectOutput ) ;
      }
      if ( '' != this._snapshot )
      {
         needCompress = true ;
         this._collectSnapshot( db, collectOutput ) ;
      }
   } catch ( e )
   {
      throw e ;
   } finally
   {
      if ( null != db )
      {
         db.close() ;
      }
   }

   try
   {
      if ( needCompress )
      {
         if ( 'zip' != this._compress )
         {
            cmd.run( 'cd ' + collectOutput + '/../ && tar -zcf ' + collectOutputDir + '.tar.gz ' + collectOutputDir );
         } else
         {
            cmd.run( 'cd ' + collectOutput + '/../ && zip -r ' + collectOutputDir + '.zip ' + collectOutputDir );
         }
      } else
      {
         File.mkdir( collectOutput, 0777 ) ;
      }
   } catch ( e )
   {
      setLastErrMsg( 'Failed to compress files "' + collectOutputDir + '", error: ' + getLastErrMsg() );
      throw e ;
   }

   return collectOutput ;
}

DiagLog.prototype._analyze = function() {
   if ( '' == this._path )
   {
      setLastErrMsg( "path cannot be empty" ) ;
      throw SDB_INVALIDARG ;
   }

   try
   {
      var cmd = new Cmd() ;
      var now = new Date( new Date().getTime() + 8 * 60 * 60 * 1000 );
   } catch ( e )
   {
      throw e ;   
   }

   try {
      // remove old dir if more than 10
      var fileArray = cmd.run( 'ls -d ' + this._tmpDir + '/analyze/diaglog_*.auto 2>/dev/null' ).trimRight( '\n' ).split( '\n' ) ;
      for ( var i = 0; i < fileArray.length - 9; i++ )
      {
         cmd.run( "rm -rf " + fileArray[i] ) ;
      }
   } catch ( e ) {}

   var output = '' ;
   if ( '' != this._output )
   {
      output = this._output ;
   } else
   {
      output = this._tmpDir + '/analyze/diaglog_' +  now.toJSON().replace('Z', '').replace( 'T', '-' ) + '.auto' ;
   }

   var pathArray = this._path.split( '/' ) ;
   var countCsv = output + '/error_count.csv' ;
   var timeCsv = output + '/error_time.csv' ;

   try {
      try
      {
         File.mkdir( output, 0777 ) ;
      } catch ( e )
      {
         setLastErrMsg( 'Failed to mkdir ' + output + ', error: ' + getLastErrMsg() );
         throw e ;
      }

      try
      {
         if ( /.*\.zip$/.test( this._path ) )
         {
            var rc = cmd.run( 'unzip -v > /dev/null 2>&1;echo $?' ).trimRight('\n');
            if ( '0' != rc )
            {
               setLastErrMsg( 'Cannot extract the archive package, the unzip command is not found' ) ;
               throw SDB_INVALIDARG ;
            } else
            {
               cmd.run( 'unzip -o ' + this._path + ' -d ' + output ) ;
               this._path = output + '/' + pathArray[pathArray.length - 1].replace( /\.zip$/, '' ) ;
            }
         } else if  ( /.*\.tar\.gz$/.test( this._path ) )
         {
            var rc = cmd.run( 'tar --version > /dev/null 2>&1;echo $?' ).trimRight('\n');
            if ( '0' != rc )
            {
               setLastErrMsg( 'Cannot extract the archive package, the tar command is not found' ) ;
               throw SDB_INVALIDARG ;
            } else
            {
               cmd.run( 'tar -xzf ' + this._path + ' -C ' + output ) ;
               this._path = output + '/' + pathArray[pathArray.length - 1].replace( /\.tar.gz$/, '' ) ;
            }
         }
         var hostArray = cmd.run( 'ls ' + this._path + '/' ).trimRight( '\n' ).split( '\n' ) ;
      } catch ( e )
      {
         setLastErrMsg( 'Failed to parse path ' + this._path + ', error: ' + getLastErrMsg() );
         throw e ;
      }

      try
      {
         var countFile = new File( countCsv, 0644, SDB_FILE_READWRITE | SDB_FILE_CREATE ) ;
         countFile.truncate() ;
      } catch ( e )
      {
         setLastErrMsg( 'Failed to touch csv ' + countCsv + ', error: ' + getLastErrMsg() );
         throw e ;
      }

      try
      {
         var timeFile = new File( timeCsv, 0644, SDB_FILE_READWRITE | SDB_FILE_CREATE ) ;
         timeFile.truncate() ;
      } catch ( e )
      {
         setLastErrMsg( 'Failed to touch csv ' + timeCsv + ', error: ' + getLastErrMsg() );
         throw e ;
      }

      for ( var i = 0; i < hostArray.length; i++ )
      {
         var hostName = hostArray[i] ;
         if ( 'analyze' == hostName || 'trap_core_snapshot' == hostName ) { continue ; }
         try
         {
            var nodeFileArray = cmd.run( 'ls ' + this._path + '/' + hostName ).trimRight( '\n' ).split( '\n' ) ;
         } catch ( e )
         {
            setLastErrMsg( 'Failed to parse diaglog in directory ' + this._path + ', error: ' + getLastErrMsg() );
            throw e ;
         }
         for ( var j = 0; j < nodeFileArray.length; j++ )
         {
            var nodeFile = nodeFileArray[j] ;
            // example: data_group1_11820_1001_1000
            var nodeInfo = nodeFileArray[j].split( '_' ) ;
            if ( 5 != nodeInfo.length ) {
               setLastErrMsg( 'Failed to parse diaglog in directory ' + this._path + '/' + hostName ) ;
               throw SDB_INVALIDARG ;
            }
            var groupName = nodeInfo[1] ;
            var serviceName = nodeInfo[2] ;
            var fullFileName = this._path + "/" + hostName + "/" + nodeFile ;
            try
            {
               var timeErrorArray = cmd.run( "awk '/(rc=|rc: )-[0-9]+\\]?$/{if(NR>5) print lines[NR-5]$0} {lines[NR]=$0}' " + fullFileName + "/* | sed 's#\\([^ ]*\\) .*\\(-[0-9][0-9]*\\)\\]\\?$#\\1 \\2#g'").trimRight( '\n' ).split( '\n' ) ;
               var errorCodeRegex = new RegExp( '^-[0-9]{1,3}$' ) ;
               for ( var k = 0; k < timeErrorArray.length; k++ )
               {
                  if ( '' != timeErrorArray[k] )
                  {
                     var timeError = timeErrorArray[k].split( ' ' ) ;
                     var timeContent = timeError[0] ;
                     var errorContent = timeError[1] ;
                     if ( '' == errorContent )
                     {
                        errorContent = timeError[timeError.length - 1] ;
                        if ( ! errorCodeRegex.test( errorContent ) )
                        {
                           errorContent = '' ;
                        }
                     }

                     if ( '' != errorContent )
                     {
                        timeFile.write( hostName + ',' + serviceName + ',' + groupName + ',' + errorContent + ',' + timeContent + '\n' ) ;
                     }
                  }
               }
            } catch ( e )
            {
               setLastErrMsg( 'Failed to parse diaglog ' + fullFileName + ', error: ' + getLastErrMsg() );
               throw e ;
            }
         }
      }
      
      try
      {
         cmd.run( "awk -F',' '{print $1\" \"$2\" \"$3\" \"$4}' " + timeCsv + " | sort | uniq -c | awk '{print $2\",\"$3\",\"$4\",\"$5\",\"$1}' > " + countCsv ) ;
      } catch ( e )
      {
         setLastErrMsg( 'Failed to parse csv ' + timeCsv + ', error: ' + getLastErrMsg() );
         throw e ;  
      }

      try
      {
         cmd.run( "sort -t',' -k5,5nr -o " + countCsv + ' ' + countCsv );
         cmd.run( "sed -i '1i\\HostName,ServiceName,GroupName,Error,Count' " + countCsv ) ;
         cmd.run( "sort -t',' -k5,5r -o " + timeCsv + ' ' + timeCsv );
         cmd.run( "sed -i '1i\\HostName,ServiceName,GroupName,Error,Time' " + timeCsv ) ;
      } catch ( e )
      {
         setLastErrMsg( 'Failed to add head line on csv ' + timeCsv + ' and ' + countCsv + ', error: ' + getLastErrMsg() );
         throw e ;  
      }
   } catch ( e )
   {
      throw e;
   } finally
   {
      if ( null != timeFile )
      {
         timeFile.close() ;
      }
      if ( null != countFile )
      {
         countFile.close() ;
      }
   }

   return output ;
}

DiagLog.prototype.close = function() {
   try
   {
      if ( '' != this._logFile )
      {
         this._logFile.close() ;
      }
   } catch ( e )
   {
      throw e ;
   }
}

DiagLog.prototype._helpSearch = function()
{
   println( "   lastFile(<num>)                - Search logs from the most recent <num> diaglog files." ) ;
   println( "   lastest(<num>)                 - Search logs from the last <num> minutes." ) ;
   println( "   timeBegin('<timeStr>')         - Search the earliest time in the diaglog files." ) ;
   println( "                                    The time string format must be parsable by Date.parse()." ) ;
   println( "   timeEnd('<timeStr>')           - Search the latest time in the diaglog files." ) ;
   println( "                                    The time string format must be parsable by Date.parse()." ) ;
   println( "   error(<num>)                   - Search for the error code <num> in the Message section of the diaglog files." ) ;
   println( "   diaglevel(<0-4>)               - Filter by diaglevel, including lower levels" ) ;
   println( "                                    0=SEVERE, 1=ERROR, 2=EVENT, 3=WARNING, 4=INFO." ) ;
   println( "   keypattern('<str>')            - Search for the key pattern <str> in the Message section of the diaglog files." ) ;
   println( "   pid(<num>)                     - Search logs by pid <num>." ) ;
   println( "   tid(<num>)                     - Search logs by tid <num>." ) ;
   println( "   after(<num>)                   - The search results include the last <num> logs of the target log." ) ;
   println( "   before(<num>)                  - The search results include the first <num> logs of the target log." ) ;
   println( "   limit(<num>)                   - Limit the maximum number of returned logs." ) ;
   println( "   original()                     - Output the original logs, default is summary logs." ) ;
   println( "   output('<path>')               - The directory for search result output." ) ;
   println( "                                    If not specified, it will be generated automatically." ) ;
   println( "   path('<path>')                 - Search the directory or archive collected by DiagLog.collect()." ) ;
}

DiagLog.prototype._helpCollect = function()
{
   println( "   snasphot('<snapType>')         - Collect the snasphot, snapType:" ) ;
   println( "                                    SNAP_CSCL: $SNAPSHOT_CS $SNAPSHOT_CL $SNAPSHOT_CATA" ) ;
   println( "                                    SNAP_SYS: $SNAPSHOT_SYSTEM $SNAPSHOT_CONFIGS $SNAPSHOT_DB" ) ;
   println( "                                       $SNAPSHOT_HEALTH $SNAPSHOT_SEQUENCES $SNAPSHOT_SVCTASKS $SNAPSHOT_TASKS" ) ;
   println( "                                    SNAP_SESSION: $SNAPSHOT_SESSION $SNAPSHOT_CONTEXT" ) ;
   println( "                                    SNAP_QUERY: $SNAPSHOT_QUERIES $SNAPSHOT_LOCKWAITS $SNAPSHOT_LATCHWAITS $SNAPSHOT_TRANS" ) ;
   println( "                                       $SNAPSHOT_ACCESSPLANS $SNAPSHOT_INDEXSTATS $SNAPSHOT_TRANSDEADLOCK $SNAPSHOT_TRANSWAIT" ) ;
   println( "                                    SNAP_ALL: include all of the above" ) ;
   println( "   core()                         - Collect the core files from all nodes." ) ;
   println( "   trap()                         - Collect the trap files from all nodes." ) ;
   println( "   all()                          - Collect the core, trap files from all nodes and snapshot('SNAP_ALL')." ) ;
   println( "   compress('<mode>')             - The compression method of collect(), optional 'tar.gz' or 'zip', default is 'tar.gz'" ) ;
   println( "   path('<path>')                 - The directory for collect result output." ) ;
   println( "                                    If not specified, it will be generated automatically." ) ;
}

DiagLog.prototype._helpAnalyze = function()
{
   println( "   output('<path>')               - The directory for analyze result output." ) ;
   println( "                                    If not specified, it will be generated automatically." ) ;
   println( "   path('<path>')                 - Required parameter, analyze the directory or archive collected by DiagLog.collect()." ) ;
}

DiagLog.prototype.help = function( mode )
{
   this._showHelp = true ;
   switch( mode )
   {
      case "search":
         println( "DiagLog help on search() methods." ) ;
         this._helpSearch() ;
         break ;
      case "collect":
         println( "DiagLog help on collect() methods." ) ;
         this._helpCollect() ;
         break ;
      case "analyze":
         println( "DiagLog help on analyze() methods." ) ;
         this._helpAnalyze() ;
         break ;
      default:
         println() ;
         println( '   --Instance methods for class "DiagLog":' ) ;
         println( "   --methods for search(): " ) ;
         this._helpSearch() ;
         println();
         println( "   --methods for collect(): " ) ;
         this._helpCollect() ;
         println();
         println( "   --methods for analyze(): " ) ;
         this._helpAnalyze() ;
         println();
         println( "   --methods for cursor: " ) ;
         println( "   next(<num>)                    - Return the next <num> records of the result from search()." ) ;
         println( "   close()                        - Close the current file from search()." ) ;
         println( "   run()                          - Run with the currently set parameters" ) ;
         println( "   reset()                        - Reset all parameters." ) ;
         break ;
   }
   return this ;
}

// end IniFile







