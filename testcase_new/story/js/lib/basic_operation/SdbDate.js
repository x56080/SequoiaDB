var tmpSdbDate = {
   help: SdbDate.prototype.help,
   toString: SdbDate.prototype.toString
};
var funcSdbDate = SdbDate;
var funcSdbDatehelp = SdbDate.help;
SdbDate=function(){try{return funcSdbDate.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
SdbDate.help = function(){try{ return funcSdbDatehelp.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
SdbDate.prototype.help=function(){try{return tmpSdbDate.help.apply(this,arguments);}catch(e){commThrowError(e);}};
SdbDate.prototype.toString=function(){try{return tmpSdbDate.toString.apply(this,arguments);}catch(e){commThrowError(e);}};
