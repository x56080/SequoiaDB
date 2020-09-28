var tmpHash = {
   help: Hash.prototype.help
};
var funcHash = Hash;
var funcfileMD5 = Hash.fileMD5;
var funchelp = Hash.help;
var funcmd5 = Hash.md5;
Hash=function(){try{return funcHash.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
Hash.fileMD5 = function(){try{ return funcfileMD5.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
Hash.help = function(){try{ return funchelp.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
Hash.md5 = function(){try{ return funcmd5.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
Hash.prototype.help=function(){try{return tmpHash.help.apply(this,arguments);}catch(e){commThrowError(e);}};
