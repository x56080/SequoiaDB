var tmpSsh = {
   close: Ssh.prototype.close,
   exec: Ssh.prototype.exec,
   getLastOut: Ssh.prototype.getLastOut,
   getLastRet: Ssh.prototype.getLastRet,
   getLocalIP: Ssh.prototype.getLocalIP,
   getPeerIP: Ssh.prototype.getPeerIP,
   help: Ssh.prototype.help,
   pull: Ssh.prototype.pull,
   push: Ssh.prototype.push,
   toString: Ssh.prototype.toString
};
var funcSsh = Ssh;
var funchelp = Ssh.help;
Ssh=function(){try{return funcSsh.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
Ssh.help = function(){try{ return funchelp.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
Ssh.prototype.close=function(){try{return tmpSsh.close.apply(this,arguments);}catch(e){commThrowError(e);}};
Ssh.prototype.exec=function(){try{return tmpSsh.exec.apply(this,arguments);}catch(e){commThrowError(e);}};
Ssh.prototype.getLastOut=function(){try{return tmpSsh.getLastOut.apply(this,arguments);}catch(e){commThrowError(e);}};
Ssh.prototype.getLastRet=function(){try{return tmpSsh.getLastRet.apply(this,arguments);}catch(e){commThrowError(e);}};
Ssh.prototype.getLocalIP=function(){try{return tmpSsh.getLocalIP.apply(this,arguments);}catch(e){commThrowError(e);}};
Ssh.prototype.getPeerIP=function(){try{return tmpSsh.getPeerIP.apply(this,arguments);}catch(e){commThrowError(e);}};
Ssh.prototype.help=function(){try{return tmpSsh.help.apply(this,arguments);}catch(e){commThrowError(e);}};
Ssh.prototype.pull=function(){try{return tmpSsh.pull.apply(this,arguments);}catch(e){commThrowError(e);}};
Ssh.prototype.push=function(){try{return tmpSsh.push.apply(this,arguments);}catch(e){commThrowError(e);}};
Ssh.prototype.toString=function(){try{return tmpSsh.toString.apply(this,arguments);}catch(e){commThrowError(e);}};
