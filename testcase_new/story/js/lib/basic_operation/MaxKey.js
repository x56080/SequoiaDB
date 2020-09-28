var tmpMaxKey = {
   help: MaxKey.prototype.help,
   toString: MaxKey.prototype.toString
};
var funcMaxKey = MaxKey;
var funchelp = MaxKey.help;
MaxKey=function(){try{return funcMaxKey.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
MaxKey.help = function(){try{ return funchelp.apply( this, arguments ); } catch( e ) { commThrowError(e) } };
MaxKey.prototype.help=function(){try{return tmpMaxKey.help.apply(this,arguments);}catch(e){commThrowError(e);}};
MaxKey.prototype.toString=function(){try{return tmpMaxKey.toString.apply(this,arguments);}catch(e){commThrowError(e);}};
