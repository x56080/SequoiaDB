var tmpFile = {
   _getPermission: File.prototype._getPermission,
   chgrp: File.prototype.chgrp,
   chmod: File.prototype.chmod,
   chown: File.prototype.chown,
   close: File.prototype.close,
   copy: File.prototype.copy,
   exist: File.prototype.exist,
   find: File.prototype.find,
   getInfo: File.prototype.getInfo,
   getSize: File.prototype.getSize,
   getUmask: File.prototype.getUmask,
   help: File.prototype.help,
   isDir: File.prototype.isDir,
   isEmptyDir: File.prototype.isEmptyDir,
   isFile: File.prototype.isFile,
   list: File.prototype.list,
   md5: File.prototype.md5,
   mkdir: File.prototype.mkdir,
   move: File.prototype.move,
   read: File.prototype.read,
   readContent: File.prototype.readContent,
   readLine: File.prototype.readLine,
   remove: File.prototype.remove,
   seek: File.prototype.seek,
   setUmask: File.prototype.setUmask,
   stat: File.prototype.stat,
   toString: File.prototype.toString,
   truncate: File.prototype.truncate,
   write: File.prototype.write,
   writeContent: File.prototype.writeContent,
   _read: File.prototype._read,
   _write: File.prototype._write,
   _truncate: File.prototype._truncate,
   _readLine: File.prototype._readLine,
   _readContent: File.prototype._readContent,
   _writeContent: File.prototype._writeContent,
   _close: File.prototype._close,
   _seek: File.prototype._seek,
   _getInfo: File.prototype._getInfo,
   _toString: File.prototype._toString
};
var funcFile = File;
var func_getPermission = File._getPermission;
var funcchgrp = File.chgrp;
var funcchmod = File.chmod;
var funcchown = File.chown;
var funccopy = File.copy;
var funcexist = File.exist;
var funcfind = File.find;
var funcgetSize = File.getSize;
var funcgetUmask = File.getUmask;
var funchelp = File.help;
var funcisDir = File.isDir;
var funcisEmptyDir = File.isEmptyDir;
var funcisFile = File.isFile;
var funclist = File.list;
var funcmd5 = File.md5;
var funcmkdir = File.mkdir;
var funcmove = File.move;
var funcremove = File.remove;
var funcscp = File.scp;
var funcsetUmask = File.setUmask;
var funcstat = File.stat;
var func_getFileObj = File._getFileObj;
var func_readFile = File._readFile;
var func_getPathType = File._getPathType;
var func_getUmask = File._getUmask;
var func_list = File._list;
var func_find = File._find;
File=function(){try{return funcFile.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
File._getPermission = function(){try{ return func_getPermission.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
File.chgrp = function(){try{ return funcchgrp.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
File.chmod = function(){try{ return funcchmod.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
File.chown = function(){try{ return funcchown.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
File.copy = function(){try{ return funccopy.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
File.exist = function(){try{ return funcexist.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
File.find = function(){try{ return funcfind.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
File.getSize = function(){try{ return funcgetSize.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
File.getUmask = function(){try{ return funcgetUmask.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
File.help = function(){try{ return funchelp.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
File.isDir = function(){try{ return funcisDir.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
File.isEmptyDir = function(){try{ return funcisEmptyDir.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
File.isFile = function(){try{ return funcisFile.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
File.list = function(){try{ return funclist.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
File.md5 = function(){try{ return funcmd5.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
File.mkdir = function(){try{ return funcmkdir.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
File.move = function(){try{ return funcmove.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
File.remove = function(){try{ return funcremove.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
File.scp = function(){try{ return funcscp.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
File.setUmask = function(){try{ return funcsetUmask.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
File.stat = function(){try{ return funcstat.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
File._getFileObj = function(){try{ return func_getFileObj.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
File._readFile = function(){try{ return func_readFile.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
File._getPathType = function(){try{ return func_getPathType.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
File._getUmask = function(){try{ return func_getUmask.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
File._list = function(){try{ return func_list.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
File._find = function(){try{ return func_find.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
File.prototype._getPermission=function(){try{return tmpFile._getPermission.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype.chgrp=function(){try{return tmpFile.chgrp.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype.chmod=function(){try{return tmpFile.chmod.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype.chown=function(){try{return tmpFile.chown.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype.close=function(){try{return tmpFile.close.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype.copy=function(){try{return tmpFile.copy.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype.exist=function(){try{return tmpFile.exist.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype.find=function(){try{return tmpFile.find.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype.getInfo=function(){try{return tmpFile.getInfo.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype.getSize=function(){try{return tmpFile.getSize.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype.getUmask=function(){try{return tmpFile.getUmask.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype.help=function(){try{return tmpFile.help.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype.isDir=function(){try{return tmpFile.isDir.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype.isEmptyDir=function(){try{return tmpFile.isEmptyDir.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype.isFile=function(){try{return tmpFile.isFile.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype.list=function(){try{return tmpFile.list.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype.md5=function(){try{return tmpFile.md5.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype.mkdir=function(){try{return tmpFile.mkdir.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype.move=function(){try{return tmpFile.move.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype.read=function(){try{return tmpFile.read.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype.readContent=function(){try{return tmpFile.readContent.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype.readLine=function(){try{return tmpFile.readLine.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype.remove=function(){try{return tmpFile.remove.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype.seek=function(){try{return tmpFile.seek.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype.setUmask=function(){try{return tmpFile.setUmask.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype.stat=function(){try{return tmpFile.stat.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype.toString=function(){try{return tmpFile.toString.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype.truncate=function(){try{return tmpFile.truncate.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype.write=function(){try{return tmpFile.write.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype.writeContent=function(){try{return tmpFile.writeContent.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype._read=function(){try{return tmpFile._read.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype._write=function(){try{return tmpFile._write.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype._truncate=function(){try{return tmpFile._truncate.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype._readLine=function(){try{return tmpFile._readLine.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype._readContent=function(){try{return tmpFile._readContent.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype._writeContent=function(){try{return tmpFile._writeContent.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype._close=function(){try{return tmpFile._close.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype._seek=function(){try{return tmpFile._seek.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype._getInfo=function(){try{return tmpFile._getInfo.apply(this,arguments);}catch(e){commThrowError(e);}};
File.prototype._toString=function(){try{return tmpFile._toString.apply(this,arguments);}catch(e){commThrowError(e);}};
