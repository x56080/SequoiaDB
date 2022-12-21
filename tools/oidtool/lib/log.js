/*******************************************************************************

   Copyright (C) 2011-2022 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = log.js

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who        Description
   ====== =========== ========== ==============================================
          2022/11/15  Tan Zhaobo Initial Draft

   Last Changed =

*******************************************************************************/
// Log for js file

var PDSEVERE = 0;
var PDERROR = 1;
var PDEVENT = 2;
var PDWARNING = 3;
var PDINFO = 4;
var PDDEBUG = 5;

var _LOG_NONE = -1;
var _LOG_GENERIC = -2;

var _LOG_NEW_LINE = "\n";
var _LOG_BASE_LEVEL = PDWARNING;
var _LOG_FILE_NAME = "js.log";
var _JS_LOG_FILE = System.getEWD() + "/" + _LOG_FILE_NAME;

/* *****************************************************************************
@discretion: help function for gen log message
@author: Tan Zhaobo
@parameter
@return
***************************************************************************** */
function _str_repeat(i, m) {
  for (var o = []; m > 0; o[--m] = i);
  return o.join("");
}

/* *****************************************************************************
@discretion: gen a string with the format "YYYY-MM-DD-HH:mm:ss.fff"
@author: Tan Zhaobo
@parameter
@return
   retStr[string]: a string with the format "YYYY-MM-DD-HH:mm:ss.fff"
                   to express current timestamp
***************************************************************************** */
function _genTimeStamp() {
  var retStr = null;
  var dateVar = new Date();
  var dateStr = dateVar.toLocaleDateString();
  var timeStr = dateVar.toLocaleTimeString();
  var millStr = _sprintf("%03s", "" + dateVar.getMilliseconds());
  var strs = dateStr.split("/");
  retStr =
    strs[2] + "-" + strs[0] + "-" + strs[1] + "-" + timeStr + "." + millStr;

  return retStr;
}

/* *****************************************************************************
@discretion: help function for gen log message
@author: Tan Zhaobo
@parameter
@return
***************************************************************************** */
function _sprintf() {
  var i = 0,
    a,
    f = arguments[i++],
    o = [],
    m,
    p,
    c,
    x,
    s = "";
  while (f) {
    if ((m = /^[^\x25]+/.exec(f))) {
      o.push(m[0]);
    } else if ((m = /^\x25{2}/.exec(f))) {
      o.push("%");
    } else if (
      (m =
        /^\x25(?:(\d+)\$)?(\+)?(0|'[^$])?(-)?(\d+)?(?:\.(\d+))?([b-fosuxX])/.exec(
          f
        ))
    ) {
      if ((a = arguments[m[1] || i++]) == null || a == undefined) {
        throw "Too few arguments.";
      }
      if (/[^s]/.test(m[7]) && typeof a != "number") {
        throw "Expecting number but found " + typeof a;
      }
      switch (m[7]) {
        case "b":
          a = a.toString(2);
          break;
        case "c":
          a = String.fromCharCode(a);
          break;
        case "d":
          a = parseInt(a);
          break;
        case "e":
          a = m[6] ? a.toExponential(m[6]) : a.toExponential();
          break;
        case "f":
          a = m[6] ? parseFloat(a).toFixed(m[6]) : parseFloat(a);
          break;
        case "o":
          a = a.toString(8);
          break;
        case "s":
          a = (a = String(a)) && m[6] ? a.substring(0, m[6]) : a;
          break;
        case "u":
          a = Math.abs(a);
          break;
        case "x":
          a = a.toString(16);
          break;
        case "X":
          a = a.toString(16).toUpperCase();
          break;
      }
      a = /[def]/.test(m[7]) && m[2] && a >= 0 ? "+" + a : a;
      c = m[3] ? (m[3] == "0" ? "0" : m[3].charAt(1)) : " ";
      x = m[5] - String(a).length - s.length;
      p = m[5] ? _str_repeat(c, x) : "";
      o.push(s + (m[4] ? a + p : p + a));
    } else {
      throw "Huh ?!";
    }
    f = f.substring(m[0].length);
  }
  return o.join("");
}

/* *****************************************************************************
@discretion: mimic "sprintf" in "C" simply 
@author: Tan Zhaobo
@parameter
   format[string]: e.g. "a = ?, b = ?, is it right/? "
@return
   newStr[string]: e.g. "a = 1, b = 2, is it right? "
@usage
   var str = sprintf( "a = ?, b = ?, is it right/? ", 1, 2, '?' ) ;
***************************************************************************** */
function sprintf(format) {
  var len = arguments.length;
  var strLen = format.length;
  var newStr = "";
  for (var i = 0, k = 1; i < strLen; i++) {
    var char = format.charAt(i);
    if (char == "\\" && i + 1 < strLen && format.charAt(i + 1) == "?") {
      newStr += "?";
      i++;
    } else if (char == "?" && k < len) {
      newStr += "" + arguments[k];
      ++k;
    } else {
      newStr += char;
    }
  }
  return newStr;
}

/* *****************************************************************************
@discretion: write the log message to the log file
@author: Tan Zhaobo
@parameter
   type[number]:
   infoStr[string]: the log message to write
@return void
***************************************************************************** */
function _write2File(type, infoStr) {
  var file = null;
  var logFileFullName = "";
  var errMsg = "";

  if (_LOG_NONE == type) {
    print(infoStr);
    return;
  }

  try {
    logFileFullName = _JS_LOG_FILE;
    file = new File(logFileFullName, 0644);
  } catch (e) {
    errMsg =
      "Failed to open log file[" +
      logFileFullName +
      "], rc: " +
      getLastError() +
      ", detail: " +
      getLastErrMsg();
    setLastErrMsg(errMsg);
    setLastError(SDB_SYS);
    throw SDB_SYS;
  }
  file.seek(0, "e");
  file.write(infoStr);
  file.close();
}

/* *****************************************************************************
@discretion: get the log level description
@author: Tan Zhaobo
@parameter
   level[number]: 0-5, the log level
@return the description of the log level
***************************************************************************** */
function _getPDLevelDesp(level) {
  if ("number" != typeof level || level < PDSEVERE || level > PDDEBUG)
    return "UNKNOWN";
  switch (level) {
    case PDSEVERE:
      return "SEVERE";
    case PDERROR:
      return "ERROR";
    case PDEVENT:
      return "EVENT";
    case PDWARNING:
      return "WARNING";
    case PDINFO:
      return "INFO";
    case PDDEBUG:
      return "DEBUG";
    default:
      return "UNKNOWN";
  }
}

/* *****************************************************************************
@discretion: write the log
@author: Tan Zhaobo
@date: 2022/11/15
@parameter
   type[number]: the log type, -1 for none, -2 for general log
   level[number]: 0-5, the log level
   funcName[string]: the function name PD_LOG2 is invoked
   line[number]: which line of js file current logger is in 
   file[string]: which js file current logger is in
   message[string]: the log message
@return void
***************************************************************************** */
function _PD_LOG_BASIC(type, level, funcName, line, file, message) {
  var formatStr = "";
  var logInfo = "";
  var levelStr = "";

  if ("number" == typeof level && level > _LOG_BASE_LEVEL) {
    return;
  }
  if (funcName == undefined || "string" != typeof funcName) {
    funcName = "";
  }

  if (message instanceof Error) {
    message = message.toString();
  }

  levelStr = _getPDLevelDesp(level);
  if (PDERROR >= level) {
    levelStr = "*" + levelStr;
  }

  try {
    formatStr = "%s [%5d][%5d][%7s]: %s(%s)%s";
    logInfo = _sprintf(
      formatStr,
      _genTimeStamp(),
      System.getPID(),
      System.getTID(),
      levelStr,
      message,
      file + ":" + funcName,
      _LOG_NEW_LINE
    );
  } catch (e) {
    // when error happen, use sprintf to retry
    formatStr = "? [?][?][?]: ?(?)?";
    logInfo = sprintf(
      formatStr,
      _genTimeStamp(),
      System.getPID(),
      System.getTID(),
      levelStr,
      message,
      file + ":" + funcName,
      _LOG_NEW_LINE
    );
  }
  _write2File(type, logInfo);
}

/* *****************************************************************************
Logger Class
@description: write log
@author: Tan Zhaobo
@date: 2022/11/15
usage:
   new Logger( currJsFileName ) ;     // fileName is the js file which current Logger use in,
                                      // not the log file
   Logger.log( level, message ) ;     // log to file. the 'message' argument can be
                                      // number/string/Error
   Logger.setLogPath( lobPath ) ;     // static method, setting the lob file
   Logger.setLogLevel( lobLevel ) ;   // static method, setting the lob base level
examples:
   Logger.setLogPath( "/opt/gitlab/sequoiadb/bin/sdboidtool_js.log" ) ;
   Logger.setLogLevel( PDWARNING ) ;
   var logger = new Logger( "test.js" ) ;
   logger.log( PDSEVERE, "00000" ) ;
   logger.log( PDERROR, "11111" ) ;
   logger.log( PDEVENT, "22222" ) ;
   logger.log( PDWARNING, "33333" ) ;
   logger.log( PDINFO, "44444" ) ;
   logger.log( PDDEBUG, "55555" ) ;
***************************************************************************** */
var Logger = function (currJsFileName) {
  if (currJsFileName == undefined || typeof currJsFileName != "string") {
    throw "currJsFileName must be string in Logger";
  }
  this.currJsFileName = currJsFileName;
};

Logger.prototype.log = function (level, message) {
  var funcName = "";
  if (Logger.prototype.log.caller) {
    funcName = Logger.prototype.log.caller.name;
  }
  _PD_LOG_BASIC(_LOG_GENERIC, level, funcName, 0, this.currJsFileName, message);
};

Logger.setLogPath = function (logPath) {
  Logger.logPath = logPath;
  _JS_LOG_FILE = Logger.logPath;
};

Logger.setLogLevel = function (logLevel) {
  if (typeof logLevel != "number" || logLevel < 0 || logLevel > 5) {
    throw new Error("Invalid log level: " + logLevel);
  }
  Logger.logLevel = logLevel;
  _LOG_BASE_LEVEL = Logger.logLevel;
};

/*
function main() {
   Logger.setLogPath( "/opt/gitlab/sequoiadb3/SequoiaDB/engine/tools/oidrepair/lib/sdboidtool_js.log" ) ;
   Logger.setLogLevel( PDDEBUG ) ;
   var logger = new Logger( "test.js" ) ;
   logger.log( PDSEVERE, "00000" ) ;
   logger.log( PDERROR, "11111" ) ;
   logger.log( PDEVENT, "22222" ) ;
   logger.log( PDWARNING, "33333" ) ;
   logger.log( PDINFO, "44444" ) ;
   logger.log( PDDEBUG, "55555" ) ;
   Logger.setLogLevel( PDWARNING ) ;
   logger.log( PDINFO, "AAAAA" ) ;
   logger.log( PDDEBUG, "BBBBB" ) ;

   import( "error.js" ) ;
   var errMsg = sprintf( "Can't alloc [?] bytes in cl[?]", 100, "foo.bar" ) ;
   var err = new SdbError( SDB_OOM, errMsg ) ;
   logger.log( PDERROR, err ) ;
}

main();
*/
