var tmpCipherUser = {
   _setToken: CipherUser.prototype._setToken,
   cipherFile: CipherUser.prototype.cipherFile,
   clusterName: CipherUser.prototype.clusterName,
   getCipherFile: CipherUser.prototype.getCipherFile,
   getClusterName: CipherUser.prototype.getClusterName,
   getUsername: CipherUser.prototype.getUsername,
   help: CipherUser.prototype.help,
   toString: CipherUser.prototype.toString,
   token: CipherUser.prototype.token
};
var funcCipherUser = CipherUser;
var funchelp = CipherUser.help;
CipherUser=function(){try{return funcCipherUser.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
CipherUser.help = function(){try{ return funchelp.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
CipherUser.prototype._setToken=function(){try{return tmpCipherUser._setToken.apply(this,arguments);}catch(e){commThrowError(e);}};
CipherUser.prototype.cipherFile=function(){try{return tmpCipherUser.cipherFile.apply(this,arguments);}catch(e){commThrowError(e);}};
CipherUser.prototype.clusterName=function(){try{return tmpCipherUser.clusterName.apply(this,arguments);}catch(e){commThrowError(e);}};
CipherUser.prototype.getCipherFile=function(){try{return tmpCipherUser.getCipherFile.apply(this,arguments);}catch(e){commThrowError(e);}};
CipherUser.prototype.getClusterName=function(){try{return tmpCipherUser.getClusterName.apply(this,arguments);}catch(e){commThrowError(e);}};
CipherUser.prototype.getUsername=function(){try{return tmpCipherUser.getUsername.apply(this,arguments);}catch(e){commThrowError(e);}};
CipherUser.prototype.help=function(){try{return tmpCipherUser.help.apply(this,arguments);}catch(e){commThrowError(e);}};
CipherUser.prototype.toString=function(){try{return tmpCipherUser.toString.apply(this,arguments);}catch(e){commThrowError(e);}};
CipherUser.prototype.token=function(){try{return tmpCipherUser.token.apply(this,arguments);}catch(e){commThrowError(e);}};
