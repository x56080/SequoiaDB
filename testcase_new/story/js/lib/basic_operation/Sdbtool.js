var tmpSdbtool = {
   help: Sdbtool.prototype.help
};
var funcSdbtool = Sdbtool;
var funcSdbtoolhelp = Sdbtool.help;
var funcSdbtoollistNodes = Sdbtool.listNodes;
Sdbtool=function(){try{return funcSdbtool.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
Sdbtool.help = function(){try{ return funcSdbtoolhelp.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
Sdbtool.listNodes = function(){try{ return funcSdbtoollistNodes.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
Sdbtool.prototype.help=function(){try{return tmpSdbtool.help.apply(this,arguments);}catch(e){commThrowError(e);}};
