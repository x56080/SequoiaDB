var tmpSdbtool = {
   help: Sdbtool.prototype.help
};
var funcSdbtool = Sdbtool;
var funchelp = Sdbtool.help;
var funclistNodes = Sdbtool.listNodes;
Sdbtool=function(){try{return funcSdbtool.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
Sdbtool.help = function(){try{ return funchelp.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
Sdbtool.listNodes = function(){try{ return funclistNodes.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
Sdbtool.prototype.help=function(){try{return tmpSdbtool.help.apply(this,arguments);}catch(e){commThrowError(e);}};
