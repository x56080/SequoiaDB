/**
 * Js file for checking/repairing collection oid.
 * 
 * Usage:
 * ./sdb -e ' var ACTION = "init"; ' -f sdboidtool.js
 * ./sdb -e ' var ACTION = "check"; ' -f sdboidtool.js
 * ./sdb -e ' var ACTION = "repair"; ' -f sdboidtool.js
 */
import( './conf.js' ) ;
import( './lib/log.js' ) ;
import( './lib/func.js' ) ;

const _script_version         = 2 ;
const _str_curr_ts            = getTSString( new Timestamp() ) ;
const _write_file_batch_count = 100 ;
const _diag_level             = typeof( DEBUG ) == "boolean" ? ( true == DEBUG ? 5 : 3 ) : 3 ;

const NEW_LINE                = "\n" ;
const TYPE_UNDEF              = "undefined" ;

const TOOL_NORMAL_CL_PREFIX   = "#" ;
const TOOL_NEW_CL_SUFFIX      = "_sdboidtool_new" ;
const TOOL_ORIG_CL_SUFFIX     = "_sdboidtool_orig" ;

const TOOL_ACTION_INIT        = "init" ;
const TOOL_ACTION_CHECK       = "check" ;
const TOOL_ACTION_REPAIR      = "repair" ;

var _logger                   = new Logger( "sdboidtool.js" ) ;

function _appendString( srcStr, content, alignPos ) {
   srcStr += content ;
   while ( srcStr.length < alignPos ) {
      srcStr += " " ;
   }
   srcStr += " " ;
   return srcStr ;
}

function _openFile( fileName, mode ) {
   var file = null ;
   try {
      if ( undefined != mode ) {
         file = new File( fileName, mode ) ;
      } else {
         file = new File( fileName ) ;
      }
   } catch( e ) {
      throw new Error( "Failed to open file: " + fileName + ", e: " + e ) ;
   }
   return file ;
}

function _getTimeSpent( beginTime, endTime ) {
   return ( endTime.getTime() - beginTime.getTime() ) / 1000 ;
}

function _readline( file ) {
   var retStr = null ;
   try {
      retStr = file.readLine() ;
      // exclude '\n'
      retStr = retStr.substring( 0, retStr.length - 1 ) ;
   } catch ( e ) {
      if ( SDB_EOF == e ) {
         return null ;
      } else {
         throw e ;
      }
   }
   return retStr ;
}

function _timeIsUp( beginTs, endTs ) {
   var currTsStr = getTSString( new Timestamp() ) ;
   if ( currTsStr < beginTs || currTsStr >= endTs )
   {
      return true ;
   }
   return false ;
}

function _printStack( err ) {
   if ( err instanceof Error ) {
      println(err.stack);
   } else {
      println( "Unkown error to print stack!" ) ;
   }
}

function _readAllLines( fileName ) {
   var nameLst = [] ;
   if ( File.exist( fileName ) ) {
      var fileSz = File.getSize( fileName ) ;
      var file = _openFile( fileName ) ;
      var contents = null ;
      try {
         contents = file.read( fileSz ) ;
      } finally {
         file.close() ;
      }
      if ( undefined != contents && contents.length > 0 ) {
         var arr = contents.split( '\n' ) ;
         for ( var i = 0 ; i < arr.length ; ++i ) {
            if ( "" != arr[i] ) {
               nameLst.push( arr[i] ) ;
            }
         }
      }
   }
   return nameLst ;
}

function _getCataSnap( sdb, fullName ) {
   var cursor = null ;
   var snapObj = null ;
   try {
      cursor = sdb.snapshot( SDB_SNAP_CATALOG, { "Name" : fullName } ) ;
      if ( undefined != cursor.next() ) {
         snapObj = cursor.current().toObj() ;
      } else {
         throw new Error( "Has no snapshot for cl: " + fullName ) ;
      }
   } finally {
      if ( undefined != cursor ) {
         cursor.close() ;
      }
   }
   return snapObj ;
}

function _getTotalLobs( obj ) {
   var retCnt = 0 ;
   for ( var i = 0; i < obj.Details.length; i++ ) {
      if ( obj.Details[i].TotalLobs ) {
         retCnt += obj.Details[i].TotalLobs ;
      }
   }
   return retCnt ;
}

function _hasLob( sdb, fullName ) {
   var cursor = null ;
   try {
      cursor = sdb.snapshot( SDB_SNAP_COLLECTIONS, { Name:fullName, RawData: true } ) ;
      while( null != cursor.next() ) {
         var snapObj = cursor.current().toObj() ;
         // when one of the node associated with current collection has lob,
         // let's stop and return true
         if ( _getTotalLobs( snapObj ) > 0 ) {
            return true ;
         }
      }
   } finally {
      if ( undefined != cursor ) {
         cursor.close() ;
      }
   }
   return false ;
} ;

var MaxMinCount = function () {
   this.Max = -1 ;
   this.Min = -1 ;
} ;

var GroupInfo = function () {
   this.GroupName             = "" ;
   this.Records               = -1 ;
} ;

var CLInfo = function () {
   this.Name                  = "" ; // cl full name
   // sharding info
   this.ShardingKey           = null ;
   this.ShardingType          = "" ;
   this.Partition             = -1 ;
   this.Groups                = -1 ;
   this.GroupInfo             = [] ;
   // record count
   this.TotalRecords          = -1 ;
   this.MaxRecordsInGroup     = -1 ;
   this.MinRecordsInGroup     = -1 ;
} ;

var IndexInfo = function() {
   this.name                  = null ;
   this.key                   = null ;
   this.unique                = null ;
   this.enforced              = null ;
   this.NotNull               = null ;
   this.NotArray              = null ;
   this.sortBufferSize        = 64 ;
   this.type                  = null ;
} ;

/**
 * Param
 */
var Param = function() {
   // config parameters
   this.hostName       = "" ;
   this.svcName        = "" ;
   this.user           = "" ;
   this.passwd         = "" ;
   this.beginTS        = "" ;
   this.endTS          = "" ;
   this.action         = "" ;

   // local parameters
   this.toolPath           = null ;
   this.initFilePath       = "" ;
   this.checkFilePath      = "" ;
   this.repairFilePath     = "" ;

   this.initReportPath     = "" ;
   this.checkReportPath    = "" ;
   this.repairReportPath   = "" ;

   this.jsLogFilePath      = "" ;
   this.cppLogFilePath     = "" ;
   this.diagLevel          = _diag_level ;

   // TODO: think about the default value
   this.taskNum            = 3 ;
   this.limit              = 50000 ;
} ;

Param.prototype.init = function () {
   // init connect info
   if ( typeof(coord_hostname) == TYPE_UNDEF || typeof(coord_port) == TYPE_UNDEF ||
        typeof(user) == TYPE_UNDEF || typeof(passwd) == TYPE_UNDEF ) {
      throw new Error( "Invalid hostname/port/username/passwd of coord" ) ;
   }
   this.hostName       = coord_hostname ;
   this.svcName        = coord_port ;
   this.user           = user ;
   this.passwd         = passwd ;

   // disable repair mode
   if ( typeof(ACTION) != TYPE_UNDEF && TOOL_ACTION_REPAIR == ACTION ) {
      throw new Error( "'repair' mode has been disable" ) ;
   }

   // init action
   if ( typeof(ACTION) == TYPE_UNDEF ||
        ( TOOL_ACTION_INIT != ACTION &&
         TOOL_ACTION_CHECK != ACTION &&
         TOOL_ACTION_REPAIR != ACTION ) ) {
      throw new Error( "Invalid ACTION: " + ACTION ) ;
   }
   this.action = ACTION ;

   // get install path, init script and tool path
   var installPath = Oma.getOmaInstallInfo().toObj().INSTALL_DIR  ;
   if ( installPath[installPath.length - 1] != '/' ) {
      installPath += '/' ;
   }

   // init tool path
   var filePath = installPath + "tools/oidtool/" ;
   var toolPath = filePath + "bin/sdboidtool" ;
   if ( !File.exist( toolPath ) ) {
      throw new Error( "Tool does not exist: " + toolPath ) ;
   }
   this.toolPath = toolPath ;

   // init log files
   var logPath = filePath + "log/" ;
   File.mkdir( logPath ) ;
   this.jsLogFilePath = logPath + "sdboidtool_script.log" ;
   this.cppLogFilePath = logPath + "sdboidtool.log" ;

   // init status files
   var resultPath = filePath + "result/" ;
   File.mkdir( resultPath ) ;
   this.initFilePath = resultPath + "init.result" ;
   this.checkFilePath = resultPath + "check.result" ;
   this.repairFilePath = resultPath + "repair.result" ;

   // init report files
   this.initReportPath = resultPath + "init.report" ;
   this.checkReportPath = resultPath + "check.report" ;
   this.repairReportPath = resultPath + "repair.report" ;

   // check start time and end time
   if ( typeof( begin_timestamp ) == TYPE_UNDEF || typeof( end_timestamp ) == TYPE_UNDEF ) {
      throw new Error( "Invaid run time timestamp" ) ;
   }
   this.beginTS = getTSString( Timestamp( begin_timestamp ) ) ;
   this.endTS = getTSString( Timestamp( end_timestamp ) ) ;
   if ( this.endTS <= this.beginTS ) {
      throw new Error( "Invaid timestamp, end ts is not greate than begin ts" ) ;
   }

   // init logger
   Logger.setLogLevel( this.diagLevel ) ;
   Logger.setLogPath( this.jsLogFilePath ) ;
} ;

/**
 * Initiator
 */
var Initiator = function ( sdb, param ) {
   this.sdb = sdb ;
   this.param = param ;
   this.outputFd = null ;
   this.reportFd = null ;
   this.totalCLNum = 0 ; // total number of doubtful collection
   this.totalRecordNum = 0 ; // total number of records in these collections 
   this.connObjs = new Object() ;
   this.beginTime = null ;
   this.endTime = null ;
} ;

Initiator.prototype.run = function() {
   this._init() ;
   try {
      this._run() ;
   } finally {
      this._fini() ;
   }
} ;

Initiator.prototype._init = function() {
   if ( File.exist( this.param.initFilePath ) ) {
      File.remove( this.param.initFilePath ) ;
   }
   if ( File.exist( this.param.initReportPath ) ) {
      File.remove( this.param.initReportPath ) ;
   }
   this.outputFd = _openFile( this.param.initFilePath, 0664 ) ;
   this.reportFd = _openFile( this.param.initReportPath, 0664 ) ;
} ;

Initiator.prototype._fini = function() {
   // close file
   if ( undefined != this.outputFd ) {
      this.outputFd.close() ;
   }
   if ( undefined != this.reportFd ) {
      this.reportFd.close() ;
   }
   // release connections
   for ( var key in this.connObjs ) {
      this.connObjs[key].close() ;
   }
} ;

Initiator.prototype._run = function() {
   var clInfoLst = [] ;
   var cursor = null ;
   var runCount = 0 ;

   // write header info to file
   this._writeHeadInfo() ;

   // get snapshot, and write the info of doubtful cl to file
   cursor = this.sdb.snapshot( SDB_SNAP_CATALOG ) ;
   try {
      while( cursor.next() ) {
         var snapObj = cursor.current().toObj() ;

         // check shard groups
         var groups = snapObj["CataInfo"] ;
         if ( undefined == groups || groups.length <= 1 ) {
            continue ;
         }
         // check shardingkey
         var shardingKeys = snapObj["ShardingKey"] ;
         if ( undefined == shardingKeys ) {
            continue ;
         }
         // check _id
         var oid = shardingKeys["_id"] ;
         if ( undefined == oid ) {
            continue ;
         }
         // check is main cl
         if ( undefined != snapObj["IsMainCL"] && true == snapObj["IsMainCL"] ) {
            continue ;
         }

         // set cl info
         var clInfo = new CLInfo() ;
         this._getCLMetaInfo( snapObj, clInfo ) ;
         this._getCLCountInfo( clInfo ) ;
         if ( 0 == clInfo.TotalRecords ) {
            continue ;
         }
         this._analyzeCLInfo( clInfo ) ;
         clInfoLst.push( clInfo ) ;

         // update calculation info
         this.totalRecordNum += clInfo.TotalRecords ;
         this.totalCLNum += 1 ;

         // write cl info to file by batch
         runCount++ ;
         if ( runCount % _write_file_batch_count == 0 ) {
            this._writeCLInfo( clInfoLst ) ;
            clInfoLst = [] ;
         }
      }
   } finally {
      if ( undefined != cursor ) {
         cursor.close() ;
      }
   }
   // write the final cl info to file
   this._writeCLInfo( clInfoLst ) ;

   // write summarey to file
   this._writeSummary() ;
} ;

Initiator.prototype._getCLMetaInfo = function( snapObj, clInfo ) {
   // Name
   clInfo.Name = snapObj["Name"] ;
   // ShardingKey
   clInfo.ShardingKey = snapObj["ShardingKey"] ;
   // ShardingType
   clInfo.ShardingType = snapObj["ShardingType"] ;
   // Partition
   clInfo.Partition = snapObj["Partition"] ;
   // Groups
   var groupInfoLst = snapObj["CataInfo"] ;
   clInfo.Groups = groupInfoLst.length ;
   // GroupInfo
   for ( var idx = 0 ; idx < groupInfoLst.length ; ++idx ) {
      var groupObj = groupInfoLst[idx] ;
      var groupName = groupObj["GroupName"] ;
      var groupInfo = new GroupInfo() ;
      groupInfo.GroupName = groupName ;
      clInfo.GroupInfo.push( groupInfo ) ;
   }
} ;

Initiator.prototype._getCLCountInfo = function( clInfo ) {
   // get cs name and cl name
   var fullName = clInfo.Name ;
   var csName = fullName.split(".")[0] ;
   var clName = fullName.split(".")[1] ;

   try {
      // get total record count of cl
      var cl = this.sdb.getCS( csName ).getCL( clName ) ;
      clInfo.TotalRecords = cl.count().valueOf() ;
      // get counts in groups
      var grpInfoLst = clInfo.GroupInfo ;
      for ( var idx = 0 ; idx < grpInfoLst.length ; ++idx ) {
         var groupName = grpInfoLst[idx].GroupName ;
         // get connection of this group master node
         var conn = this.connObjs[groupName] ;
         if ( undefined == conn ) {
            // create a connection for this group
            var nodeName = this.sdb.getRG( groupName ).getMaster().toString() ;
            conn = new Sdb( nodeName.split(":")[0], nodeName.split(":")[1], user, passwd ) ;
            this.connObjs[groupName] = conn ;
         }
         var count = conn.getCS( csName ).getCL( clName ).count().valueOf() ;
         grpInfoLst[idx].Records = count ;
      }
   } catch ( e ) {
      throw new Error( "Failed to get count in cl: " + fullName + ", e: " + e ) ;
   }
} ;

Initiator.prototype._analyzeCLInfo = function( clInfo ) {
   // get max and min records in groups
   var maxMinCount = this._getMaxAndMinRecordCount( clInfo.GroupInfo ) ;
   clInfo.MaxRecordsInGroup = maxMinCount.Max ;
   clInfo.MinRecordsInGroup = maxMinCount.Min ;
} ;

Initiator.prototype._getMaxAndMinRecordCount = function( grpInfoLst ) {
   var maxMinCount = new MaxMinCount() ;
   if ( 0 == grpInfoLst.length ) {
      return maxMinCount ;
   } else if ( grpInfoLst.length == 1 ) {
      maxMinCount.Max = maxMinCount.Min = grpInfoLst[0].Records ;
      return maxMinCount ;
   }
   var max = grpInfoLst[0].Records, min = grpInfoLst[0].Records ;
   for ( var pos in grpInfoLst ) {
      max = grpInfoLst[pos].Records > max ? grpInfoLst[pos].Records : max ;
      min = grpInfoLst[pos].Records < min ? grpInfoLst[pos].Records : min ;
   }
   maxMinCount.Max = max ;
   maxMinCount.Min = min ;
   return maxMinCount ;
} ;

Initiator.prototype._writeHeadInfo = function() {
   this.beginTime = new Date() ;
   // write header info of report file
   var reportInfo = "Version: " + _script_version + ", action: " + this.param.action
                    + ", report at: " + _str_curr_ts ;
   var headLine1  = "Name                                          ShardingKey             ShardingType   Groups   TotalRecords   MaxRecordsInGroup   MinRecordsInGroup" ;
   var headLine2  = "--------------------------------------------------------------------------------------------------------------------------------------------------" ;

   this.reportFd.write( reportInfo + NEW_LINE ) ;
   this.reportFd.write( NEW_LINE ) ;
   this.reportFd.write( headLine1 + NEW_LINE ) ;
   this.reportFd.write( headLine2 + NEW_LINE ) ;
} ;

Initiator.prototype._writeCLInfo = function( clInfoLst ) {
   // write cl info to both .init and .report files
   for ( var pos in clInfoLst ) { 
      var clInfo = clInfoLst[pos] ;
      this.outputFd.write( clInfo.Name + NEW_LINE ) ;
      this.reportFd.write( this._formatInfo( clInfo ) + NEW_LINE ) ;
   }
} ;

Initiator.prototype._formatInfo = function ( clInfo ) {
   var content = "" ;
   // Name
   content = _appendString( content, clInfo.Name, 45 ) ;
   // ShardingKey
   content = _appendString( content, JSON.stringify( clInfo.ShardingKey ), 69 ) ;
   // ShardingType
   content = _appendString( content, clInfo.ShardingType, 84 ) ;
   // Groups
   content = _appendString( content, clInfo.Groups, 93 ) ;
   // TotalRecords
   content = _appendString( content, clInfo.TotalRecords, 108 ) ;
   // MaxRecordsInGroup
   content = _appendString( content, clInfo.MaxRecordsInGroup, 128 ) ;
   // MinRecordsInGroup
   content = _appendString( content, clInfo.MinRecordsInGroup, 148 ) ;
   return content ;
} ;

Initiator.prototype._writeSummary = function() {
   // write summary to report file
   this.reportFd.write( NEW_LINE ) ;
   this.reportFd.write( "Summary:" + NEW_LINE ) ;
   var summary = "" ;
   summary = "Total number of collections which need to be checked is: " + this.totalCLNum ;
   this.reportFd.write( summary + NEW_LINE ) ;
   summary = "Total number of records: " + this.totalRecordNum ;
   this.reportFd.write( summary + NEW_LINE ) ;
   this.endTime = new Date() ;
   var timeSpent = _getTimeSpent( this.beginTime, this.endTime ) ;
   summary = "Total time spent: " + timeSpent + " seconds" ;
   this.reportFd.write( summary + NEW_LINE ) ;
} ;

/**
 * Checher
 */
var Checker = function( sdb, param ) {
   this.sdb = sdb ;
   this.param = param ;
   this.cmd = new Cmd() ;
   this.inputFd = null ;
   this.outputFd = null ;
   this.reportFd = null ;
   this.doneList = [] ;
   // values for statistics
   this.beginTime = getTSString( new Timestamp() ) ;
   this.endTime = "" ;
   this.totalOpNum = 0 ;   // the number of cl have been handled
   this.totalOpSucc = 0 ;
   this.totalOpFail = 0 ;
   this.finishAll = false ;   // all the cl in init.result have been checking successfully or not
} ;

Checker.prototype.run = function() {
   this._init() ;
   try {
      this._run() ;
   } finally {
      this._fini() ;
   }
} ;

Checker.prototype._init = function() {
   if ( !File.exist( this.param.initFilePath ) ) {
      throw new Error("init file does not exist") ;
   }
   if ( File.exist( this.param.checkReportPath ) ) {
      File.remove( this.param.checkReportPath ) ;
   }

   // get the cl which has been check
   this.doneList = _readAllLines( this.param.checkFilePath ) ;

   // prepare file handles
   this.inputFd = _openFile( this.param.initFilePath, 0664 ) ;
   this.outputFd = _openFile( this.param.checkFilePath, 0664 ) ;
   this.reportFd = _openFile( this.param.checkReportPath, 0664 ) ;

   // when output file exist, append the ouput to the final
   if ( File.getSize( this.param.checkFilePath ) > 0 ) {
      this.outputFd.seek( 0, 'e' ) ;
   }
} ;

Checker.prototype._fini = function() {
   // close files
   if ( undefined != this.inputFd ) {
      this.inputFd.close() ;
   }
   if ( undefined != this.outputFd ) {
      this.outputFd.close() ;
   }
   if ( undefined != this.reportFd ) {
      this.reportFd.close() ;
   }
} ;

Checker.prototype._run = function() {
   // write head info
   this._writeHeadInfo() ;
   try {
      while( true ) {
         // get cl for checking
         var fullName = this._readCLName() ;
         if ( undefined == fullName ) {
            // when has no cl to check, let's see has error happened or not,
            // if not, that means we have check all the cl, let's stop
            this.finishAll = this.totalOpFail > 0 ? false : true ;
            break ;
         }
         if ( this._timeIsUp() ) {
            _logger.log( PDEVENT, "Time is up, stop running" ) ;
            break ;
         }
         // skip the empty line
         if ( "" == fullName ) {
            continue ;
         }
         // if it's the cl which we had finished checking, skip it
         if ( ( -1 != this.doneList.indexOf( fullName ) ) ||
              ( -1 != this.doneList.indexOf( TOOL_NORMAL_CL_PREFIX + fullName ) ) ) {
            _logger.log( PDEVENT, "Skip cl: " + fullName ) ;
            continue ;
         }
         // check cl
         try {
            this._checkCL( fullName ) ;
         } catch ( e ) {
            _logger.log( PDERROR, "Failed to check cl : " + fullName +
                         ", e: " + e + ", stack: " + e.stack ) ;
         }
      }
   } finally {
      // writet summary info
      this._writeSummary() ;
   }
} ;

Checker.prototype._readCLName = function() {
   return _readline( this.inputFd ) ;
} ;

Checker.prototype._timeIsUp = function() {
   return _timeIsUp( this.param.beginTS, this.param.endTS ) ;
} ;

Checker.prototype._writeHeadInfo = function() {
   // write header info of report file
   var reportInfo = "Version: " + _script_version + ", action: " + this.param.action
                    + ", report at: " + _str_curr_ts ; 
   this.reportFd.write( reportInfo + NEW_LINE ) ;
   this.reportFd.write( NEW_LINE ) ;
} ;

Checker.prototype._writeCLInfo = function( outputStr, reportStr ) {
   if ( undefined != outputStr ) {
      this.outputFd.write( outputStr + NEW_LINE ) ;
   }
   if ( undefined != reportStr ) {
      this.reportFd.write( reportStr + NEW_LINE ) ;
   }
} ;

Checker.prototype._writeSummary = function() {
   var summary = "" ;
   this.endTime = getTSString( new Timestamp() ) ;
   this.reportFd.write( NEW_LINE ) ;
   this.reportFd.write( "Summary: " + NEW_LINE ) ;
   this.reportFd.write( "Begin: " + this.beginTime + NEW_LINE ) ;
   this.reportFd.write( "End  : " + this.endTime + NEW_LINE ) ;
   summary = "Total check: " + this.totalOpNum + ", success: " + this.totalOpSucc
              + ", failed: " + this.totalOpFail + " collection(s)" ;
   this.reportFd.write( summary + NEW_LINE ) ;
   summary = "Has check all collections in init.result file: " + this.finishAll ;
   this.reportFd.write( summary + NEW_LINE ) ;
} ;

Checker.prototype._checkCL = function( fullName ) {
   var outputStr = null ;
   var reportStr = null ;
   var beginTime = new Date() ;

   try {
      reportStr = fullName ;
      // check records
      var checkResult = null ;
      try {
         // need to check all the group or not
         var checkAll = this._getShardingKeyNum( fullName ) > 1 ? true : false ;
         checkResult = this._checkRecords( fullName, checkAll ) ;
      } catch ( e ) {
         var err = new Error( "Failed to check cl : " + fullName + ", e: " + e ) ;
         _logger.log( PDERROR, err ) ;
         throw err ;
      }
      if ( undefined == checkResult ) {
         var err = new Error( "Invalid check result: " + checkResult + " for cl: " + fullName ) ;
         _logger.log( PDERROR, err ) ;
         throw err ;
      }
      // get ErrNo
      var errNo = checkResult["ErrNo"] ;
      if ( typeof( errNo ) != "number" ) {
         var err = new Error( "Invaid ErrNo: " + errNo + " in cl: " + fullName ) ;
         _logger.log( PDERROR, err ) ;
         throw err ;
      }
      if ( SDB_OK != errNo ) {
         var err = new Error( "Failed to check cl: " + fullName + " check result is: "
                              + JSON.stringify( checkResult ) ) ;
         _logger.log( PDERROR, err ) ;
         throw err ;
      }
      // get Name
      if ( fullName != checkResult["Name"] ) {
         var err = new Error( "Invaid return cl name: " + checkResult["Name"] + " in cl: " + fullName ) ;
         _logger.log( PDERROR, err ) ;
         throw err ;
      }
      // get HasAbnormalRecord
      var hasAbnormalRecord = checkResult["HasAbnormalRecord"] ;
      if ( typeof( hasAbnormalRecord ) != "boolean" ) {
         var err = new Error( "Invaid boolean value: " + hasAbnormalRecord + " in cl: " + fullName ) ;
         _logger.log( PDERROR, err ) ;
         throw err ;
      }
      // set the info which will write to files
      outputStr = hasAbnormalRecord ? fullName : TOOL_NORMAL_CL_PREFIX + fullName ;
      reportStr += " check success, data has abnormal record: " + hasAbnormalRecord ;
      this.totalOpSucc += 1 ;
   } catch ( e ) {
      reportStr += " check failed" ;
      this.totalOpFail += 1 ;
      throw e ;
   } finally {
      var timeSpent = _getTimeSpent( beginTime, new Date() ) ;
      reportStr += ", takes: " + timeSpent + " second(s)" ;
      this._writeCLInfo( outputStr, reportStr ) ;
      this.totalOpNum += 1 ;
   }
} ;

Checker.prototype._getShardingKeyNum = function( fullName ) {
   var snapObj = _getCataSnap( this.sdb, fullName ) ;
   return Object.getOwnPropertyNames( snapObj.ShardingKey ).length ;
} ;

/**
 * check the records in the specified collection
 * @param {string} fullName the cl full name
 * @param {boolean} checkAll check all the groups or not
 * @returns CheckResult obj: 
 *          { ErrNo: 0, Name: "foo.bar", Action:"check", HasAbnormalRecords: false }
 */
Checker.prototype._checkRecords = function( fullName, checkAll ) {
   var resultObj = null ;
   var retMsg = null ;
   var user = this.param.user ;
   var passwd = this.param.passwd ;
   var command = "" ;
   var debug = this.param.diagLevel == PDDEBUG ? "true" : "false" ; ;

   // convert argument to string for sdboidtool
   if ( this.param.user == "" ) {
      user = "\"\"" ;
   }
   if ( this.param.passwd == "" ) {
      passwd = "\"\"" ;
   }
   // build command
   command = this.param.toolPath + 
             " --hostname " + this.param.hostName + " --svcname " + this.param.svcName +
             " --user " + user + " --password " + passwd +
             " --srcclfullname " + fullName +
             " --action " + "check" +
             " --logpath " + this.param.cppLogFilePath +
             " --checkall " + checkAll +
             " --debug " + debug ;
   _logger.log( PDDEBUG, "Run command is: " + command + " for cl: " + fullName ) ;
   try {
      retMsg = this.cmd.run( command ) ;
   } catch ( e ) {
      throw new Error( "Failed to run check command, e: " + e + ", msg: " + getLastErrMsg() ) ;
   }
   _logger.log( PDDEBUG, "Run command return: " + retMsg + " for cl: " + fullName ) ;
   resultObj = eval( "(" + retMsg + ")" ) ;
   return resultObj ;
} ;

/**
 * RepairContext
 */
 var RepairContext = function ( sdb, param, fullName ) {
   // common info
   this.sdb                   = sdb ;
   this.param                 = param ;
   this.cmd                   = new Cmd() ;
   this.cs                    = null ;
   // cl base info
   this.fullName              = fullName ;
   this.csName                = fullName.split(".")[0] ;
   this.clName                = fullName.split(".")[1] ;
   this.newCLFullName         = null ;
   this.newCLName             = null ; // not a full name
   this.origCLBakFullName     = null ;
   this.origCLBakName         = null ; // not a full name
   // cl ddl info
   this.option                = null ;
   this.indexDefs             = null ;
   // sub cl attach info
   this.mainCLFullName        = null ;
   this.boundInfo             = null ;
   // control info
   this._beginCount           = -1 ;
   this._beginLsnSum          = -1 ;
   this._needStop             = false ;
   this._hasRenameNewCL       = false ;
} ;

RepairContext.prototype.needStop = function() { return this._needStop ; } ;

RepairContext.prototype.init = function () {
   var fullName = this.fullName ;
   var csName = this.csName ;
   var clName = this.clName ;
   this.cs = this.sdb.getCS( this.csName ) ;
   this._beginCount = this.cs.getCL( clName ).count().valueOf() ;
   this._beginLsnSum = this._getLsnSum() ;

   // init new cl name and original cl backup name
   this.newCLName = clName + TOOL_NEW_CL_SUFFIX ;
   this.newCLFullName = csName + "." + this.newCLName ;
   this.origCLBakName = clName + TOOL_ORIG_CL_SUFFIX ;
   this.origCLBakFullName = csName + "." + this.origCLBakName ;

   // get catalog snapshot of cl
   var snapObj = this._getCataSnap( fullName ) ;
   // get cl ddl option
   var clOption = this._getCLOption( snapObj ) ;
   this.option = clOption ;

   // if it's sub cl, get it's main cl and attach bound info
   if ( undefined != snapObj.MainCLName ) {
      this.mainCLFullName = snapObj.MainCLName ;
      this.boundInfo      = this._getSubCLBound( snapObj.MainCLName, fullName ) ;
   }

   // get cl index defines
   var cl = this.cs.getCL( clName ) ;
   this.indexDefs = this._getIdxDefs( cl ) ;

   // check has not support cl option or index or not
   if ( this._hasNotSupportOption( clOption ) || this._hasNotSupportIdx( this.indexDefs ) ) {
      var err = new Error( "The conditions for creating a new cl are not met in cl: " + fullName ) ;
      _logger.log( PDERROR, err ) ;
      throw err ;
   }

   // check original cl has lob or not
   if ( _hasLob( this.sdb, fullName ) ) {
      var err = new Error( "Not support to handle cl: " + fullName + " which has lob" )
      _logger.log( PDERROR, err ) ;
      throw err ;
   }
} ;

RepairContext.prototype.run = function() {
   // create new cl
   this._createCL() ;

   // import data
   this._importData() ;

   // create index
   this._createIndexes() ;

   // check count/lsn/lob in original cl
   this._checkOrigCLStat() ;

   // rename original cl
   this._renameOrigCL() ;

   // rename new cl
   this._renameNewCL() ;

   // drop the backup of original cl
   this._dropOrigCL() ;
} ;

RepairContext.prototype._createCL = function() {
   var fullName = this.fullName ;
   var newCLName = this.newCLName ;
   var clOption = this.option ;

   // check new cl has exist nor not
   if ( this._isCLExist( newCLName ) ) {
      var err = new Error( "The creating cl: " + this.newCLFullName +
                           " had existed, please checking" ) ;
      _logger.log( PDERROR, err ) ;
      // unexpected case, stop to check
      this._needStop = true ;
      throw err ;
   }

   // create new cl
   try {
      this.cs.createCL( newCLName, clOption ) ;
      _logger.log( PDEVENT, "Success to create new cl: " + newCLName + " in cs: " + this.csName +
                            " with option: " + JSON.stringify( clOption ) ) ;
   } catch ( e ) {
      var err = new Error( "Failed to create new cl: " + newCLName + " in cs: " + this.csName +
                           " with the option: " + JSON.stringify( clOption ) + ", e: " + e ) ;
      _logger.log( PDERROR, err ) ;
      this._dropNewCL() ;
      throw err ;
   }

   // check and reset sharding info in new cl
   try {
      this._checkAndResetShardInfo() ;
   } catch ( e ) {
      var err = new Error( "Failed to check and reset sharding info in new cl for: " +
                           fullName + ", e: " + e ) ;
      _logger.log( PDERROR, err ) ;
      this._dropNewCL() ;
      throw err ;
   }
} ;

RepairContext.prototype._importData = function() {
   var fullName = this.fullName ;
   var newCLFullName = this.newCLFullName ;
   var beginTime = new Date() ;

   try {
      // reload records
      var retObj = null ;
      try {
         retObj = this._reloadRecords( fullName, newCLFullName ) ;
      } catch ( e ) {
         var err = new Error( "Failed to import data to cl : " + newCLFullName + ", e: " + e ) ;
         _logger.log( PDERROR, err ) ;
         throw err ;
      }
      if ( undefined == retObj ) {
         var err = new Error(  "Invalid reload result: " + retObj + " for cl: " + fullName ) ;
         _logger.log( PDERROR, err ) ;
         throw err ;
      }
      // get ErrNo
      var errNo = retObj["ErrNo"] ;
      if ( typeof( errNo ) != "number" ) {
         var err = new Error( "Invaid return ErrNo: " + errNo + " in cl: " + fullName ) ;
         _logger.log( PDERROR, err ) ;
         throw err ;
      }
      if ( SDB_OK != errNo ) {
         var err = new Error( "Failed to reload cl: " + fullName + ", result obj is: " +
                              JSON.stringify( retObj ) ) ;
         _logger.log( PDERROR, err ) ;
         throw err ;
      }
      // check return cl full name
      if ( fullName != retObj["Name"] ) {
         var err = new Error( "Invaid return cl name: " + retObj["Name"] + " in cl: " + fullName ) ;
         _logger.log( PDERROR, err ) ;
         throw err ;
      }
   } catch( e ) {
      this._dropNewCL() ;
      throw e ;
   } finally {
      var timeSpent = _getTimeSpent( beginTime, new Date() ) ;
      _logger.log( PDEVENT, "Reload cl: " + fullName + " takes: " + timeSpent + " second(s)" ) ;
   }
} ;

RepairContext.prototype._createIndexes = function() {
   var newCL = this.cs.getCL( this.newCLName ) ;
   var origIdxInfoLst = this.indexDefs ;
   var currIdxInfoLst = this._getIdxDefs( newCL ) ;
   var existedIdxNames = this._getIdxNames( currIdxInfoLst ) ;

   for ( var i = 0 ; i < origIdxInfoLst.length ; ++i ) {
      var idxInfo = origIdxInfoLst[i] ;
      // when the index had existed in the new cl, let's ignore it
      if ( -1 != existedIdxNames.indexOf( idxInfo.name ) ) {
         continue ;
      }
      // create index
      var idxOption = this._buildIdxOption( idxInfo ) ;
      try {
         newCL.createIndex( idxInfo.name, idxInfo.key, idxOption ) ;
      } catch ( e ) {
         _logger.log( PDERROR, "Failed to create index: " +
                      JSON.stringify( idxInfo ) + " for cl: " + this.newCLName + ", e: " + e ) ;
         this._needStop = true ;
         this._dropNewCL() ;
         throw e ;
      }
      _logger.log( PDEVENT, "Success to create index: " + 
                   JSON.stringify( idxInfo ) + " for cl: " + this.newCLName ) ;
   }
} ;

RepairContext.prototype._checkOrigCLStat = function() {
   var cl = this.cs.getCL( this.clName ) ;
   // check count
   var currCount = cl.count().valueOf() ;
   if ( this._beginCount != currCount ) {
      var err = new Error( "Record count of cl: " + this.fullName +
                           " has changed from: " + this._beginCount + " to " + currCount +
                           ", stop repairing current cl" ) ;
      _logger.log( PDERROR, err ) ;
      this._dropNewCL() ;
      throw err ;
   }
   // check lob count
   // these no lob at the beginning, so listLobs() will not cause large IO read here
   var lobCount = cl.listLobs().size() ;
   if ( 0 != lobCount ) {
      var err = new Error( "Lob count of cl: " + this.fullName +
                           " has changed from: 0 to " + lobCount +
                           ", stop repairing current cl" ) ;
      _logger.log( PDERROR, err ) ;
      this._dropNewCL() ;
      throw err ;
   }
   // check lsn
   var currLsnSum = this._getLsnSum() ;
   _logger.log( PDDEBUG, "begin lsn sum: " + this._beginLsnSum + ", curr lsn sum: " +
                currLsnSum + " in cl: " + this.fullName ) ;
   if ( this._beginLsnSum != currLsnSum ) {
      var err = new Error( "Lsn sum of cl: " + this.fullName +
                           " has changed from: " + this._beginLsnSum + " to " + currLsnSum +
                           ", stop repairing current cl" ) ;
      _logger.log( PDERROR, err ) ;
      this._dropNewCL() ;
      throw err ;
   }
} ;

RepairContext.prototype._renameOrigCL = function() {
   // detach from main cl, if it is sub cl
   if ( undefined != this.mainCLFullName ) {
      try {
         this._detachFromMainCL() ;
         _logger.log( PDEVENT, "Detach cl: " + this.fullName + " from main cl: " +
                               this.mainCLFullName + " before rename" ) ;
      } catch ( e ) {
         this._needStop = true ;
         this._dropNewCL() ;
         throw e ;
      }
   }
   // rename original cl to backup cl
   try {
      this._renameCL( this.clName, this.origCLBakName ) ;
   } catch ( e ) {
      this._needStop = true ;
      // drop newly created cl
      var err = new Error( "Failed to rename original cl: " + this.fullName +
                           " to backup name, e: " + e ) ;
      _logger.log( PDERROR, err ) ;
      this._dropNewCL() ;
      // if it's sub cl, attach it back to main cl
      if ( undefined != this.mainCLFullName ) {
         try {
            this._attachToMainCL() ;
            _logger.log( PDEVENT, "Success attaching cl: " + this.fullName +
                                  " back to main cl: " + this.mainCLFullName ) ;
         } catch ( e2 ) {
            _logger.log( PDERROR, "Failed to attach cl: " + this.fullName +
                                  " back to main cl: " + this.mainCLFullName ) ;
         }
      }
      throw err ;
   }
} ;

RepairContext.prototype._renameNewCL = function() {
   // rename new cl to the original cl name
   try {
      this._renameCL( this.newCLName, this.clName ) ;
      this._hasRenameNewCL = true ;
   } catch ( e ) {
      this._dropNewCL() ;
      this._recoverOrigCL() ;
      throw e ;
   }
   // attach to main cl, if it is sub cl
   if ( undefined != this.mainCLFullName ) {
      try {
         this._attachToMainCL() ;
         _logger.log( PDEVENT, "Attach cl: " + this.fullName +
                               " to main cl: " + this.mainCLFullName + " after rename" ) ;
      } catch ( e ) {
         this._dropNewCL() ;
         this._recoverOrigCL() ;
         throw e ;
      }
   }
} ;

RepairContext.prototype._dropOrigCL = function() {
   if ( !this._isCLExist( this.origCLBakName ) ) {
      var err = new Error( "Original backup cl: " + this.origCLBakName +
                           " does not exist, can't drop it" ) ;
      _logger.log( PDERROR, err ) ;
      this._needStop = true ;
      throw err ;
   }
   // drop the original backup cl
   try {
      this.cs.dropCL( this.origCLBakName, { SkipRecycleBin: true } ) ;
   } catch ( e ) {
      var err = new Error( "Failed to drop original backup cl: " + this.origCLBakName +
                           ", e: " + e ) ;
      _logger.log( PDERROR, err ) ;
      this._needStop = true ;
      throw err ;
   }
} ;

RepairContext.prototype._isCLExist = function( clName ) {
   try {
      this.cs.getCL( clName ) ;
      return true ;
   } catch ( e ) {
      if ( SDB_DMS_NOTEXIST == e ) {
         return false ;
      } else {
         throw new Error( "Failed to check cl: " + clName + " exist in cs: " + cs +
                          " or not, e: " + e ) ;
      }
   }
} ;

RepairContext.prototype._getCataSnap = function( fullName ) {
   return _getCataSnap( this.sdb, fullName ) ;
} ;

RepairContext.prototype._dropNewCL = function() {
   var dropName = this._hasRenameNewCL ? this.clName : this.newCLName ;
   try {
      // sanity check
      // when has renamed new cl, make sure the original backup cl exists,
      // before drop the newly created cl
      if ( this._hasRenameNewCL && !this._isCLExist( this.origCLBakName ) ) {
         var err = new Error( "Original backup cl: " + this.origCLBakName +
                              " does not exist, can't drop cl: " + dropName ) ;
         _logger.log( PDERROR, err ) ;
         this._needStop = true ;
         throw err ;
      }
      // drop the newly created cl
      this.cs.dropCL( dropName, { SkipRecycleBin: true } ) ;
   } catch ( e ) {
      if ( SDB_DMS_NOTEXIST != e ) {
         _logger.log( PDERROR, "Failed to drop the newly created cl: " + dropName + " in cs: " +
                      this.csName + ", _hasRenameNewCL is: " + this._hasRenameNewCL + " e: " + e ) ;
      }
   }
} ;

RepairContext.prototype._getCLOption = function( snapObj ) {
   var option = new Object() ;

   if ( undefined == snapObj ) {
      throw new Error( "Empty snapshot object" ) ;
   }

   // build cl option
   // ShardingKey
   if ( undefined != snapObj.ShardingKey ) {
      option.ShardingKey = snapObj.ShardingKey ;
   }
   // ShardingType
   if ( undefined != snapObj.ShardingType ) {
      option.ShardingType = snapObj.ShardingType ;
   }
   // Partition
   if ( undefined != snapObj.Partition ) {
      option.Partition = snapObj.Partition ;
   }
   // ReplSize
   if ( undefined != snapObj.ReplSize ) {
      option.ReplSize = snapObj.ReplSize ;
   }
   // Compressed
   if ( undefined != snapObj.Attribute ) {
      option.Compressed = ( snapObj.Attribute & 0x00000001 ) > 0 ? true : false ;
   }
   // CompressionType
   if ( undefined != snapObj.CompressionTypeDesc ) {
      option.CompressionType = snapObj.CompressionTypeDesc ;
   }
   // AutoSplit
   if ( undefined != snapObj.AutoSplit ) {
      option.AutoSplit = snapObj.AutoSplit ;
   }
   // Group: set it later
   // AutoIndexId
   if ( undefined != snapObj.Attribute && ( ( snapObj.Attribute & 0x00000002 ) > 0 ) ) {
      option.AutoIndexId = false ;
   }
   // EnsureShardingIndex
   if ( undefined != snapObj.EnsureShardingIndex ) {
      option.EnsureShardingIndex = snapObj.EnsureShardingIndex ;
   }
   // StrictDataMode
   if ( undefined != snapObj.Attribute && ( ( snapObj.Attribute & 0x00000008 ) > 0 ) ) {
      option.StrictDataMode = true ;
   }
   // AutoIncrement
   if ( undefined != snapObj.AutoIncrement ) {
      // not support, set invalid AutoIncrement value
      option.AutoIncrement = snapObj.AutoIncrement ;
   }
   // LobShardingKeyFormat
   if ( undefined != snapObj.LobShardingKeyFormat ) {
      // not support, use with IsMainCL
      option.LobShardingKeyFormat = snapObj.LobShardingKeyFormat ;
   }
   // IsMainCL
   if ( undefined != snapObj.IsMainCL ) {
      // not support, initializer has filtered this type of cl
      option.IsMainCL = snapObj.IsMainCL ;
   }
   // DataSource
   if ( undefined != snapObj.DataSourceID ) {
      // not support, mark the current cl has DataSource, not going to use this value
      option.DataSource = "Tmp" ;
   }
   // Mapping
   if ( undefined != snapObj.Mapping ) {
      // not support
      option.Mapping = snapObj.Mapping ;
   }
   return option ;
} ;

RepairContext.prototype._getLsnSum = function() {
   var lsnSum = 0 ;
   var preGrpName = "" ;
   var cond = { "Name" : this.fullName, "RawData" : true } ;
   var sel = { "Name" : "", "Details.GroupName" : "",
               "Details.DataCommitLSN" : "", "Details.IndexCommitLSN" : "" } ;
   var sort = { "Details.GroupName" : 1 } ;

   var cursor = this.sdb.snapshot( SDB_SNAP_COLLECTIONS, cond, sel, sort ) ;
   var obj = null ;
   try {
      while ( undefined != ( obj = cursor.next() ) ) {
         // obj is:
         // { "Name": "test33.test", "Details": [ { "GroupName": "db1", "DataCommitLSN": 1498929048, "IndexCommitLSN": 1498927400 } ] }
         var detailObjLst = obj.toObj()["Details"] ;
         var detailObj = detailObjLst[0] ;
         var groupName = detailObj["GroupName"] ;
         if ( preGrpName == groupName ) {
            // if it's the node in the same group, let's ignore current node
            continue ;
         } else {
            preGrpName = groupName ;
            var dataLsn = detailObj["DataCommitLSN"] ;
            var idxLsn = detailObj["IndexCommitLSN"] ;
            lsnSum += dataLsn + idxLsn ;
         }
      }
   } finally {
      if ( undefined != cursor ) {
         cursor.close() ;
      }
   }
   return lsnSum ;
} ;

RepairContext.prototype._getSubCLBound = function( mainCLName, subCLName ) {
   var boundObj = new Object() ;

   // get sub cl bound info
   var snapObj = this._getCataSnap( mainCLName ) ;
   var cataInfoLst = snapObj.CataInfo ;
   for ( var i = 0 ; i < cataInfoLst.length ; ++i ) {
      var cataInfo = cataInfoLst[i] ;
      if ( subCLName == cataInfo.SubCLName ) {
         boundObj.LowBound = cataInfo.LowBound ;
         boundObj.UpBound = cataInfo.UpBound ;
         break ;
      }
   }

   // sanity check
   if ( 0 == Object.getOwnPropertyNames( boundObj ).length ) {
      var err = new Error( "Main cl: " + mainCLName + " has no bound info for sub cl: " + subCLName ) ;
      _logger.log( PDERROR, err ) ;
      throw err ;
   }
   return boundObj ;
} ;

RepairContext.prototype._hasNotSupportOption = function( option ) {
   // not support _id as a sharding key of range partition
   if ( "range" == option.ShardingType ) {
      _logger.log( PDERROR, "Not support _id as a sharding key of range partition" ) ;
      return true ;
   }

   // not support AutoIncrement
   if ( undefined != option.AutoIncrement ) {
      _logger.log( PDERROR, "Not support AutoIncrement" ) ;
      return true ;
   }
   // not support DataSource or Mapping
   if ( undefined != option.DataSourceID || undefined != option.Mapping ) {
      _logger.log( PDERROR, "Not support DataSource" ) ;
      return true ;
   }
   // not support IsMainCL and LobShardingKeyFormat
   if ( undefined != option.IsMainCL || undefined != option.LobShardingKeyFormat ) {
      _logger.log( PDERROR, "Not support IsMainCL or LobShardingKeyFormat" ) ;
      return true ;
   }
   return false ;
} ;

RepairContext.prototype._hasNotSupportIdx = function( idxInfoLst ) {
   var hasTextIdx = false ;
   var hasIdIdx = false ;
   for ( var i = 0 ; i < idxInfoLst.length ; ++i ) {
      var idxInfo = idxInfoLst[i] ;
      if ( "Text" == idxInfo.type ) {
         hasTextIdx = true ;
      }
      if ( "$id" == idxInfo.name ) {
         hasIdIdx = true ;
      }
   }
   // not support fulltext
   if ( hasTextIdx ) {
      _logger.log( PDERROR, "Not support fulltext index" ) ;
      return true ;
   }
   // when has no id index, we can't split
   if ( !hasIdIdx ) {
      _logger.log( PDERROR, "Not has $id index" ) ;
      return true ;
   }
   return false ;
} ;

RepairContext.prototype._getIdxDefs = function( cl ) {
   var idxInfoLst = [] ;
   var cursor = cl.listIndexes() ;
   var obj = null ;
   try {
      while ( undefined != ( obj = cursor.next() ) ) {
         var idxObj = obj.toObj() ;
         var idxDef = idxObj.IndexDef ;
         var idxInfo = new IndexInfo() ;
         idxInfo.name = idxDef.name ;
         idxInfo.key = idxDef.key ;
         idxInfo.unique = idxDef.unique ;
         idxInfo.enforced = idxDef.enforced ;
         idxInfo.NotNull = idxDef.NotNull ;
         idxInfo.NotArray = idxDef.NotArray ;
         idxInfo.type = idxObj.Type ;
         idxInfoLst.push( idxInfo ) ;
      }
   } finally {
      if ( undefined != cursor ) {
         cursor.close() ;
      }
   }
   return idxInfoLst ;
} ;

RepairContext.prototype._getIdxNames = function( idxInfoLst ) {
   var idxNameLst = [] ;
   for ( var i = 0 ; i < idxInfoLst.length ; ++i ) {
      idxNameLst.push( idxInfoLst[i].name ) ;
   }
   return idxNameLst ;
} ;

RepairContext.prototype._buildIdxOption = function( idxInfo ) {
   var option = new Object() ;
   option.Unique = idxInfo.unique ;
   option.Enforced = idxInfo.enforced ;
   option.NotNull = idxInfo.NotNull ;
   option.NotArray = idxInfo.NotArray ;
   option.SortBufferSize = idxInfo.sortBufferSize ;
   return option ;
} ;

RepairContext.prototype._checkAndResetShardInfo = function() {
   // check the new cl has the same sharding info with the original cl or not
   var origSnapObj = this._getCataSnap( this.fullName ) ;
   var newSnapObj = this._getCataSnap( this.newCLFullName ) ;
   if ( !this._compareShardInfo( origSnapObj, newSnapObj ) ) {
      this._resetShardInfo( origSnapObj, newSnapObj ) ;
   }
} ;

/**
 * check the new cl sharding info is the same with the original cl's or not
 * @param {object} origSnapObj 
 * @param {object} newSnapObj 
 * @returns true for equal, false for not
 */
 RepairContext.prototype._compareShardInfo = function( origSnapObj, newSnapObj ) {
   var origCataInfoLst = origSnapObj["CataInfo"] ;
   var newCataInfoLst = newSnapObj["CataInfo"] ;

   // compare length
   if ( origCataInfoLst.length != newCataInfoLst.length ) {
      return false ;
   }
   // compare group names
   var origGrpNameLst = this._getGroupNames( origSnapObj ) ;
   var newGrpNameLst = this._getGroupNames( newSnapObj ) ;
   if ( origGrpNameLst.length != newGrpNameLst.length ) {
      return fasle ;
   }
   for ( var i = 0 ; i < origGrpNameLst.length ; ++i ) {
      if ( -1 == newGrpNameLst.indexOf( origGrpNameLst[i] ) ) {
         // when new cl list does not contain the group exsited in original cl, return fasle
         return false ;
      }
   }
   // compare low bound and up bound
   var origShardInfoLst = this._getShardInfo( origSnapObj ) ;
   var newShardInfoLst = this._getShardInfo( newSnapObj ) ;
   if ( origShardInfoLst.length != newShardInfoLst.length ) {
      return false ;
   }
   for ( var i = 0 ; i < origShardInfoLst.length ; ++i ) {
      var origShardInfo = origShardInfoLst[i] ;
      for ( var j = 0 ; j < newShardInfoLst.length ; ++j ) {
         var newShardInfo = newShardInfoLst[j] ;
         if ( origShardInfo.GroupName == newShardInfo.GroupName && 
              ( origShardInfo.LowBound != newShardInfo.LowBound ||
                origShardInfo.UpBound != newShardInfo.UpBound ) ) {
            // TODO: test it by tdd
            return false ;
         }
      }
   }
   return true ;
} ;

RepairContext.prototype._resetShardInfo = function( origSnapObj, newSnapObj ) {
   var srcGrpName = null ;
   var destGrpName = null ;
   var newCL = this.cs.getCL( this.newCLName ) ;
   // get group names
   var origGrpNameLst = this._getGroupNames( origSnapObj ) ;
   var newGrpNameLst = this._getGroupNames( newSnapObj ) ;

   // split all the shards to the first group of original cl
   destGrpName = origGrpNameLst[0] ;
   for ( var i = 0 ; i < newGrpNameLst.length ; ++i ) {
      srcGrpName = newGrpNameLst[i] ;
      if ( destGrpName != srcGrpName ) {
         try {
            newCL.split( srcGrpName, destGrpName, 100 ) ;
         } catch ( e ) {
            var err = Error( "Failed to 100% split from: " + srcGrpName + " to: " + destGrpName ) ;
            _logger.log( PDERROR, err ) ;
            throw err ;
         }
      }
   }

   // according to the original sharding bounds, split the new cl
   srcGrpName = origGrpNameLst[0] ;
   destGrpName = null ;
   var shardInfoLst = this._getShardInfo( origSnapObj ) ;
   for ( var i = 0 ; i < shardInfoLst.length ; ++i ) {
      destGrpName = shardInfoLst[i].GroupName ;
      var lowBound = shardInfoLst[i].LowBound ;
      var upBound = shardInfoLst[i].UpBound ;
      if ( srcGrpName != destGrpName ) {
         try {
            newCL.split( srcGrpName, destGrpName, lowBound, upBound ) ;
         } catch ( e ) {
            var err = Error( "Failed to split from: " + srcGrpName + " to: " + destGrpName +
                           ", lowBound is: " + JSON.stringify( lowBound ) +
                           ", upBound is: " + JSON.stringify( upBound ) ) ;
            _logger.log( PDERROR, err ) ;
            throw err ;
         }
      }
   }
} ;

RepairContext.prototype._getGroupNames = function( snapObj ) {
   var grpNameLst = [] ;
   var cataInfoLst = snapObj["CataInfo"] ;
   for ( var i = 0 ; i < cataInfoLst.length ; ++i ) {
      grpNameLst.push( cataInfoLst[i].GroupName ) ;
   }
   return grpNameLst ;
} ;

/**
 * get shard info from catalog snapshot
 * @param {object} snapObj 
 * @return list of sharding info.
 * e.g. [{GroupName: "db1", LowBound: {Partition:0}, UpBound: {Partition:2048}},
 *       {GroupName: "db2", LowBound: {Partition:2048}, UpBound: {Partition:4096}}]
 */
RepairContext.prototype._getShardInfo = function( snapObj ) {
   var shardInfoLst = [] ;
   var cataInfoLst = snapObj["CataInfo"] ;
   for ( var i = 0 ; i < cataInfoLst.length ; ++i ) {
      var cataInfo = cataInfoLst[i] ;
      var shardInfo = new Object() ;
      shardInfo.GroupName = cataInfo.GroupName ; ;
      shardInfo.LowBound = new Object() ;
      shardInfo.UpBound = new Object() ;
      shardInfo.LowBound.Partition = cataInfo.LowBound[""] ;
      shardInfo.UpBound.Partition = cataInfo.UpBound[""] ;
      shardInfoLst.push( shardInfo ) ;
   }
   return shardInfoLst ;
} ;

RepairContext.prototype._recoverOrigCL = function() {
   // check original backup cl exist or not
   if ( !this._isCLExist( this.origCLBakName ) ) {
      var err = new Error( "Original backup cl: " + this.origCLBakName +
                           " does not exist, can't recover it" ) ;
      _logger.log( PDERROR, err ) ;
      this._needStop = true ;
      throw err ;
   }
   // check the original cl name has been used or not
   if ( this._isCLExist( this.clName ) ) {
      var err = new Error( "Original cl name: " + this.clName +
                           " has been used, can't recover original backup cl" ) ;
      _logger.log( PDERROR, err ) ;
      this._needStop = true ;
      throw err ;
   }

   // rename original backup cl name to original cl name
   try {
      this._renameCL( this.origCLBakName, this.clName ) ;
   } catch ( e ) {
      this._needStop = true ;
      var err = new Error( "Failed to rename original backup cl: " + this.origCLBakName +
                           " back to: " + this.fullName + ", e: " + e ) ;
      _logger.log( PDERROR, err ) ;
      throw err ;
   }

   // attach cl, if it is sub cl
   if ( undefined != this.mainCLFullName ) {
      try {
         this._attachToMainCL() ;
      } catch ( e ) {
         this._needStop = true ;
         _logger.log( PDERROR, "Failed to attach to main cl: " + this.mainCLFullName +
                               ", when recovering original cl: " + this.fullName ) ;
         throw e ;
      }
   }
} ;

RepairContext.prototype._renameCL = function( name, newName ) {
   try {
      this.cs.renameCL( name, newName ) ;
   } catch ( e ) {
      var err = new Error( "Failed to rename cl from: " + name + " to: " + newName +
                           " in cs: " + this.csName + ", e: " + e ) ;
      _logger.log( PDERROR, err ) ;
      throw err ;
   }
} ;

RepairContext.prototype._attachToMainCL = function() {
   try {
      var mainCL = this.sdb.getCS( this.mainCLFullName.split(".")[0] ).
                              getCL( this.mainCLFullName.split(".")[1] ) ;
      mainCL.attachCL( this.fullName, this.boundInfo ) ;
   } catch ( e ) {
      var err = new Error( "Failed to attach sub cl: " + this.fullName + " to main cl: " +
                           this.mainCLFullName + ", it's bound is: " +
                           JSON.stringify( this.boundInfo ) + ", e: " + e ) ;
      _logger.log( PDERROR, err ) ;
      throw err ;
   }
} ;

RepairContext.prototype._detachFromMainCL = function() {
   try {
      var mainCL = this.sdb.getCS( this.mainCLFullName.split(".")[0] ).
                              getCL( this.mainCLFullName.split(".")[1] ) ;
      mainCL.detachCL( this.fullName ) ;
   } catch ( e ) {
      var err = new Error( "Failed to detach sub cl: " + this.fullName + " from main cl: " +
                           this.mainCLFullName + ", it's bound is: " +
                           JSON.stringify( this.boundInfo ) + ", e: " + e ) ;
      _logger.log( PDERROR, err ) ;
      throw err ;
   }
} ;

RepairContext.prototype._getSubCLInfo = function() {
   var obj = new Object() ;
   obj.Name = this.fullName ;
   obj.MainCLName = this.mainCLFullName ;
   obj.BoundInfo = this.boundInfo ;
   return obj ;
} ;

/**
 * reload the records from src to dest cl
 * @param {string} srcFullName the src cl full name
 * @param {string} destFullName the dest cl full name
 * @returns CheckResult obj: 
 *          { ErrNo: 0, Name: "foo.bar", Action: "repair" }
 */
 RepairContext.prototype._reloadRecords = function( srcFullName, destFullName ) {
   var resultObj = null ;
   var retMsg = null ;
   var user = this.param.user ;
   var passwd = this.param.passwd ;
   var command = "" ;
   var debug = this.param.diagLevel == PDDEBUG ? "true" : "false" ; ;

   // convert argument to string for sdboidtool
   if ( this.param.user == "" ) {
      user = "\"\"" ;
   }
   if ( this.param.passwd == "" ) {
      passwd = "\"\"" ;
   }
   // build command
   command = this.param.toolPath + 
             " --hostname " + this.param.hostName + " --svcname " + this.param.svcName +
             " --user " + user + " --password " + passwd +
             " --srcclfullname " + srcFullName +
             " --destclfullname " + destFullName +
             " --action " + "repair" +
             " --logpath " + this.param.cppLogFilePath +
             " --tasknum " + this.param.taskNum +
             " --limit " + this.param.limit +
             " --debug " + debug ;
   _logger.log( PDDEBUG, "Run command is: " + command + " for cl: " + srcFullName ) ;
   try {
      retMsg = this.cmd.run( command ) ;
   } catch ( e ) {
      throw new Error( "Failed to run import command, e: " + e + ", msg: " + getLastErrMsg() ) ;
   }
   _logger.log( PDDEBUG, "Run command return: " + retMsg + " for cl: " + srcFullName ) ;
   resultObj = eval( "(" + retMsg + ")" ) ;
   return resultObj ;
} ;

/**
 * Repairer
 */
var Repairer = function( sdb, param ) {
   this.sdb = sdb ;
   this.param = param ;
   this.inputFd = null ;
   this.outputFd = null ;
   this.reportFd = null ;
   this.doneList = [] ;
   // values for statistics
   this.beginTime = getTSString( new Timestamp() ) ;
   this.endTime = "" ;
   this.totalOpNum = 0 ;    // the number of cl have been handled
   this.totalOpSucc = 0 ;
   this.totalOpFail = 0 ;
   this.finishAll = false ; // all the cl in check.result have been repairing successfully or not
} ;

Repairer.prototype.run = function() {
   this._init() ;
   try {
      this._run() ;
   } finally {
      this._fini() ;
   }
} ;

Repairer.prototype._init = function() {
   if ( !File.exist( this.param.checkFilePath ) ) {
      throw new Error( "Check list file does not exist" ) ;
   }
   // remove the existed report file, and make a new one
   if ( File.exist( this.param.repairReportPath ) ) {
      File.remove( this.param.repairReportPath ) ;
   }
   // get the cl which has been repaired
   this.doneList = _readAllLines( this.param.repairFilePath ) ;

   // prepare file handles
   this.inputFd = _openFile( this.param.checkFilePath, 0664 ) ;
   this.outputFd = _openFile( this.param.repairFilePath, 0664 ) ;
   this.reportFd = _openFile( this.param.repairReportPath, 0664 ) ;

   // when output file exist, append the ouput to the final
   if ( File.getSize( this.param.repairFilePath ) > 0 ) {
      this.outputFd.seek( 0, 'e' ) ;
   }
} ;

Repairer.prototype._fini = function() {
   // close files
   if ( undefined != this.inputFd ) {
      this.inputFd.close() ;
   }
   if ( undefined != this.outputFd ) {
      this.outputFd.close() ;
   }
   if ( undefined != this.reportFd ) {
      this.reportFd.close() ;
   }
} ;

Repairer.prototype._run = function() {
   // write head info
   this._writeHeadInfo() ;
   try {
      while( true ) {
         // get cl for repairing
         var fullName = this._readCLName() ;
         if ( undefined == fullName ) {
            // when has no cl to repair, let's see has error happened or not,
            // if not, that means we have repair all the cl, let's stop
            this.finishAll = this.totalOpFail > 0 ? false : true ;
            break ;
         }
         if ( this._timeIsUp() ) {
            _logger.log( PDEVENT, "Time is up, stop running" ) ;
            break ;
         }
         // skip the empty line
         if ( "" == fullName ) {
            continue ;
         }
         // if it's the cl which has no abnormal records, skip it
         if ( TOOL_NORMAL_CL_PREFIX == fullName[0] ) {
            _logger.log( PDEVENT, "Skip cl: " + fullName ) ;
            continue ;
         }
         // if it's the cl which we had finished repairing, skip it
         if ( -1 != this.doneList.indexOf( fullName ) ) {
            _logger.log( PDEVENT, "Skip cl: " + fullName ) ;
            continue ;
         }
         // repair cl
         _logger.log( PDEVENT, "Begin to repair cl: " + fullName ) ;
         try {
         this._repairCL( fullName ) ;
         } finally {
            _logger.log( PDEVENT, "Finish repairing cl: " + fullName ) ;
         }
      }
   } finally {
      // writet summary info
      this._writeSummary() ;
   }
} ;

Repairer.prototype._readCLName = function() {
   return _readline( this.inputFd ) ;
} ;

Repairer.prototype._timeIsUp = function() {
   return _timeIsUp( this.param.beginTS, this.param.endTS ) ;
} ;

Repairer.prototype._repairCL = function( fullName ) {
   var resultStr = null ;
   var reportStr = null ;
   var beginTime = new Date() ;
   var context = new RepairContext( this.sdb, this.param, fullName ) ;
   try {
      context.init() ;
      context.run() ;
      this.totalOpSucc += 1 ;
      resultStr = fullName ;
      reportStr = fullName + " repair success" ;
   } catch ( e ) {
      this.totalOpFail += 1 ;
      reportStr = fullName + " repair failed" ;
      _logger.log( PDERROR, "Failed to repair cl: " + fullName +
                            ", e: " + e + ", stack: " + e.stack ) ;
      if ( context.needStop() ) {
         _logger.log( PDSEVERE, "Stop repairing, please checking" ) ;
         throw e ;
      }
   } finally {
      var timeSpent = _getTimeSpent( beginTime, new Date() ) ;
      reportStr += ", takes " + timeSpent + " seconds(s)" ;
      this._writeCLInfo( resultStr, reportStr ) ;
      this.totalOpNum += 1 ;
   }
} ;

Repairer.prototype._writeHeadInfo = function() {
   // write header info of report file
   var reportInfo = "Version: " + _script_version + ", action: " + this.param.action
                    + ", report at: " + _str_curr_ts ; 
   this.reportFd.write( reportInfo + NEW_LINE ) ;
   this.reportFd.write( NEW_LINE ) ;
} ;

Repairer.prototype._writeCLInfo = function( outputStr, reportStr ) {
   if ( undefined != outputStr ) {
      this.outputFd.write( outputStr + NEW_LINE ) ;
   }
   if ( undefined != reportStr ) {
      this.reportFd.write( reportStr + NEW_LINE ) ;
   }
} ;

Repairer.prototype._writeSummary = function() {
   var summary = "" ;
   this.endTime = getTSString( new Timestamp() ) ;
   this.reportFd.write( NEW_LINE ) ;
   this.reportFd.write( "Summary: " + NEW_LINE ) ;
   this.reportFd.write( "Begin: " + this.beginTime + NEW_LINE ) ;
   this.reportFd.write( "End  : " + this.endTime + NEW_LINE ) ;
   summary = "Total repair: " + this.totalOpNum + ", success: " + this.totalOpSucc
              + ", failed: " + this.totalOpFail + " collection(s)" ;
   this.reportFd.write( summary + NEW_LINE ) ;
   summary = "Has repaired all collections in check.result file: " + this.finishAll ;
   this.reportFd.write( summary + NEW_LINE ) ;
} ;

function run( sdb, param ) {
   if ( TOOL_ACTION_INIT == param.action ) {
      // init
      var initiator = new Initiator( sdb, param ) ;
      initiator.run() ;
   } else if ( TOOL_ACTION_CHECK == param.action ) {
      // check
      var checker = new Checker( sdb, param ) ;
      checker.run() ;
   } else if ( TOOL_ACTION_REPAIR == param.action ) {
      // repair
      var repairer = new Repairer( sdb, param ) ;
      repairer.run() ;
   } else {
      throw new Error( "Unkown ACTION: " + ACTION ) ;
   }
}

function main() {
   var param = new Param() ;
   param.init() ;
   // connect to coord
   var sdb = new Sdb( param.hostName, param.svcName, param.user, param.passwd ) ;
   try {
      _logger.log( PDEVENT, "##### Begin to run(" + param.action + ") #####" ) ;
      run( sdb, param ) ;
   } finally {
      _logger.log( PDEVENT, "----- Finish running(" + param.action + ") -----" ) ;
      sdb.close() ;
   }
}

try {
   main() ;
   println( "Finish running, see the output files in ./result and ./log for more information!" ) ;
} catch ( e ) {
   println( "Failed to run!" ) ;
   _printStack( e ) ;
   throw e ;
}
