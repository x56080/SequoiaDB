var tmpFileContent = {
   clear: FileContent.prototype.clear,
   getLength: FileContent.prototype.getLength,
   help: FileContent.prototype.help,
   toBase64Code: FileContent.prototype.toBase64Code
};
var funcFileContent = FileContent;
var funchelp = FileContent.help;
FileContent=function(){try{return funcFileContent.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
FileContent.help = function(){try{ return funchelp.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
FileContent.prototype.clear=function(){try{return tmpFileContent.clear.apply(this,arguments);}catch(e){commThrowError(e);}};
FileContent.prototype.getLength=function(){try{return tmpFileContent.getLength.apply(this,arguments);}catch(e){commThrowError(e);}};
FileContent.prototype.help=function(){try{return tmpFileContent.help.apply(this,arguments);}catch(e){commThrowError(e);}};
FileContent.prototype.toBase64Code=function(){try{return tmpFileContent.toBase64Code.apply(this,arguments);}catch(e){commThrowError(e);}};
