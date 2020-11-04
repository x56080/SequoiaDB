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
var funcFile_getPermission = File._getPermission;
var funcFilechgrp = File.chgrp;
var funcFilechmod = File.chmod;
var funcFilechown = File.chown;
var funcFilecopy = File.copy;
var funcFileexist = File.exist;
var funcFilefind = File.find;
var funcFilegetSize = File.getSize;
var funcFilegetUmask = File.getUmask;
var funcFilehelp = File.help;
var funcFileisDir = File.isDir;
var funcFileisEmptyDir = File.isEmptyDir;
var funcFileisFile = File.isFile;
var funcFilelist = File.list;
var funcFilemd5 = File.md5;
var funcFilemkdir = File.mkdir;
var funcFilemove = File.move;
var funcFileremove = File.remove;
var funcFilescp = File.scp;
var funcFilesetUmask = File.setUmask;
var funcFilestat = File.stat;
var funcFile_getFileObj = File._getFileObj;
var funcFile_readFile = File._readFile;
var funcFile_getPathType = File._getPathType;
var funcFile_getUmask = File._getUmask;
var funcFile_list = File._list;
var funcFile_find = File._find;
File=function(){try{return funcFile.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
File._getPermission = function(){try{ return funcFile_getPermission.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
File.chgrp = function(){try{ return funcFilechgrp.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
File.chmod = function(){try{ return funcFilechmod.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
File.chown = function(){try{ return funcFilechown.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
File.copy = function(){try{ return funcFilecopy.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
File.exist = function(){try{ return funcFileexist.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
File.find = function(){try{ return funcFilefind.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
File.getSize = function(){try{ return funcFilegetSize.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
File.getUmask = function(){try{ return funcFilegetUmask.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
File.help = function(){try{ return funcFilehelp.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
File.isDir = function(){try{ return funcFileisDir.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
File.isEmptyDir = function(){try{ return funcFileisEmptyDir.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
File.isFile = function(){try{ return funcFileisFile.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
File.list = function(){try{ return funcFilelist.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
File.md5 = function(){try{ return funcFilemd5.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
File.mkdir = function(){try{ return funcFilemkdir.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
File.move = function(){try{ return funcFilemove.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
File.remove = function(){try{ return funcFileremove.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
File.scp = function(){try{ return funcFilescp.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
File.setUmask = function(){try{ return funcFilesetUmask.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
File.stat = function(){try{ return funcFilestat.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
File._getFileObj = function(){try{ return funcFile_getFileObj.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
File._readFile = function(){try{ return funcFile_readFile.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
File._getPathType = function(){try{ return funcFile_getPathType.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
File._getUmask = function(){try{ return funcFile_getUmask.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
File._list = function(){try{ return funcFile_list.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
File._find = function(){try{ return funcFile_find.apply( this, arguments ); } catch( e ) { var msg = e.message || e; throw new Error(msg) } };
File.prototype._getPermission=function(){try{return tmpFile._getPermission.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype.chgrp=function(){try{return tmpFile.chgrp.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype.chmod=function(){try{return tmpFile.chmod.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype.chown=function(){try{return tmpFile.chown.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype.close=function(){try{return tmpFile.close.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype.copy=function(){try{return tmpFile.copy.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype.exist=function(){try{return tmpFile.exist.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype.find=function(){try{return tmpFile.find.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype.getInfo=function(){try{return tmpFile.getInfo.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype.getSize=function(){try{return tmpFile.getSize.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype.getUmask=function(){try{return tmpFile.getUmask.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype.help=function(){try{return tmpFile.help.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype.isDir=function(){try{return tmpFile.isDir.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype.isEmptyDir=function(){try{return tmpFile.isEmptyDir.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype.isFile=function(){try{return tmpFile.isFile.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype.list=function(){try{return tmpFile.list.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype.md5=function(){try{return tmpFile.md5.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype.mkdir=function(){try{return tmpFile.mkdir.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype.move=function(){try{return tmpFile.move.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype.read=function(){try{return tmpFile.read.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype.readContent=function(){try{return tmpFile.readContent.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype.readLine=function(){try{return tmpFile.readLine.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype.remove=function(){try{return tmpFile.remove.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype.seek=function(){try{return tmpFile.seek.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype.setUmask=function(){try{return tmpFile.setUmask.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype.stat=function(){try{return tmpFile.stat.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype.toString=function(){try{return tmpFile.toString.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype.truncate=function(){try{return tmpFile.truncate.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype.write=function(){try{return tmpFile.write.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype.writeContent=function(){try{return tmpFile.writeContent.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype._read=function(){try{return tmpFile._read.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype._write=function(){try{return tmpFile._write.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype._truncate=function(){try{return tmpFile._truncate.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype._readLine=function(){try{return tmpFile._readLine.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype._readContent=function(){try{return tmpFile._readContent.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype._writeContent=function(){try{return tmpFile._writeContent.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype._close=function(){try{return tmpFile._close.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype._seek=function(){try{return tmpFile._seek.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype._getInfo=function(){try{return tmpFile._getInfo.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
File.prototype._toString=function(){try{return tmpFile._toString.apply(this,arguments);}catch(e){var msg = e.message || e; throw new Error(msg);}};
