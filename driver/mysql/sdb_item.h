/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   
*******************************************************************************/
#ifndef SDB_ITEM__H
#define SDB_ITEM__H

#include <include/client.hpp>
#include <item.h>
#include <item_cmpfunc.h>
#include "sdb_err_code.h"

class sdb_item : public Sql_alloc
{
public:
   sdb_item():
   is_finished( FALSE )
   {
   }
   virtual ~sdb_item(){} ;

   virtual int push( sdb_item *cond_item ) { return SDB_ERR_COND_UNEXPECTED_ITEM ; }
   virtual int push( Item *cond_item ) { return SDB_ERR_COND_UNEXPECTED_ITEM ; }
   virtual int to_bson( bson::BSONObj &obj ) = 0 ;
   virtual const char *name() = 0 ;
   virtual bool finished() { return is_finished ;} ;

   virtual Item_func::Functype type() = 0 ;

protected:
   bool                             is_finished ;
} ;

class sdb_logic_item : public sdb_item
{
public:
   sdb_logic_item() { is_ok = TRUE ; }
   virtual int push( sdb_item *cond_item ) ;
   virtual int push( Item *cond_item ) ;
   virtual int to_bson( bson::BSONObj &obj ) ;
   virtual const char *name() = 0 ;
   virtual Item_func::Functype type() = 0 ;

protected:
   bson::BSONArrayBuilder           children ;
   bool                             is_ok ;
};

class sdb_and_item : public sdb_logic_item
{
public:
   sdb_and_item() {}
   virtual ~sdb_and_item() {}

   virtual int to_bson( bson::BSONObj &obj ) ;
   virtual Item_func::Functype type() { return Item_func::COND_AND_FUNC ; }
   virtual const char *name() { return "$and" ; }
} ;

class sdb_or_item : public sdb_logic_item
{
public:
   sdb_or_item() {}
   virtual ~sdb_or_item(){}

   virtual Item_func::Functype type() { return Item_func::COND_OR_FUNC ; }
   virtual const char *name() { return "$or" ; }
} ;

class sdb_func_item : public sdb_item
{
public:
   sdb_func_item() ;
   virtual ~sdb_func_item() ;

   virtual int push( sdb_item *cond_item ) ;
   virtual int push( Item *cond_item ) ;
   virtual int pop( Item *&para_item ) ;
   virtual void update_stat() ;
   virtual int get_item_val( const char *field_name,
                             Item *item_val,
                             Field *field,
                             bson::BSONObj &obj,
                             bson::BSONArrayBuilder *arr_builder = NULL ) ;
   virtual int to_bson( bson::BSONObj &obj ) = 0 ;
   virtual const char *name() = 0 ;
   virtual Item_func::Functype type() = 0 ;

   uint get_para_num(){ return para_num_max ; }

protected:
   List<Item >                      para_list ;
   uint                             para_num_cur ;
   uint                             para_num_max ;
   sdb_item                         *l_child ;
   sdb_item                         *r_child ;
} ;

class sdb_func_unkown : public sdb_func_item
{
public:
   sdb_func_unkown( Item_func *item ) ;
   ~sdb_func_unkown() ;

   virtual int to_bson( bson::BSONObj &obj ) { return SDB_ERR_COND_UNKOWN_ITEM ; }
   virtual const char *name() { return "unkown" ; }
   virtual Item_func::Functype type() { return Item_func::UNKNOWN_FUNC ; }

   Item *get_func_item() { return func_item ; }

private:
   Item_func                     *func_item ;
};

class sdb_func_unary_op : public sdb_func_item
{
public:
   sdb_func_unary_op() ;
   ~sdb_func_unary_op() ;

   virtual int to_bson( bson::BSONObj &obj ) = 0 ;
   virtual const char *name() = 0 ;
   virtual Item_func::Functype type() = 0 ;
   
};

class sdb_func_isnull : public sdb_func_unary_op
{
public:
   sdb_func_isnull() ;
   ~sdb_func_isnull() ;

   virtual int to_bson( bson::BSONObj &obj ) ;
   virtual const char *name() { return "$isnull" ; }
   virtual Item_func::Functype type() { return Item_func::ISNULL_FUNC ; }
};

class sdb_func_isnotnull : public sdb_func_unary_op
{
public:
   sdb_func_isnotnull() ;
   ~sdb_func_isnotnull() ;

   virtual int to_bson( bson::BSONObj &obj ) ;
   virtual const char *name() { return "$isnull" ; }
   virtual Item_func::Functype type() { return Item_func::ISNOTNULL_FUNC ; }
};

class sdb_func_bin_op : public sdb_func_item
{
public:
   sdb_func_bin_op() ;
   ~sdb_func_bin_op() ;

   virtual int to_bson( bson::BSONObj &obj ) = 0 ;
   virtual const char *name() = 0 ;
   virtual Item_func::Functype type() = 0 ;
};

class sdb_func_cmp : public sdb_func_bin_op
{
public:
   sdb_func_cmp() ;
   ~sdb_func_cmp() ;

   virtual int to_bson( bson::BSONObj &obj ) ;
   int to_bson_with_child( bson::BSONObj &obj ) ;
   virtual const char *name() = 0 ;
   virtual const char *inverse_name() = 0 ;
   virtual Item_func::Functype type() = 0 ;
};

class sdb_func_eq : public sdb_func_cmp
{
public:
   sdb_func_eq(){} ;
   ~sdb_func_eq(){} ;

   virtual const char *name() { return "$et" ; }
   virtual const char *inverse_name() { return "$et" ; }
   virtual Item_func::Functype type() { return Item_func::EQ_FUNC ; }
};

class sdb_func_ne : public sdb_func_cmp
{
public:
   sdb_func_ne(){} ;
   ~sdb_func_ne(){} ;

   virtual const char *name() { return "$ne" ; }
   virtual const char *inverse_name() { return "$ne" ; }
   virtual Item_func::Functype type() { return Item_func::NE_FUNC ; }
};

class sdb_func_lt : public sdb_func_cmp
{
public:
   sdb_func_lt(){} ;
   ~sdb_func_lt(){} ;

   virtual const char *name() { return "$lt" ; }
   virtual const char *inverse_name() { return "$gt" ; }
   virtual Item_func::Functype type() { return Item_func::LT_FUNC ; }
};

class sdb_func_le : public sdb_func_cmp
{
public:
   sdb_func_le(){} ;
   ~sdb_func_le(){} ;

   virtual const char *name() { return "$lte" ; }
   virtual const char *inverse_name() { return "$gte" ; }
   virtual Item_func::Functype type() { return Item_func::LE_FUNC ; }
};

class sdb_func_gt : public sdb_func_cmp
{
public:
   sdb_func_gt(){} ;
   ~sdb_func_gt(){} ;

   virtual const char *name() { return "$gt" ; }
   virtual const char *inverse_name() { return "$lt" ; }
   virtual Item_func::Functype type() { return Item_func::GT_FUNC ; }
};

class sdb_func_ge : public sdb_func_cmp
{
public:
   sdb_func_ge(){} ;
   ~sdb_func_ge(){} ;

   virtual const char *name() { return "$gte" ; }
   virtual const char *inverse_name() { return "$lte" ; }
   virtual Item_func::Functype type() { return Item_func::GE_FUNC ; }
};

class sdb_func_neg : public sdb_func_item
{
public:
   sdb_func_neg( bool has_not ) : negated(has_not) {}
   virtual ~sdb_func_neg() {}

   virtual int to_bson( bson::BSONObj &obj ) = 0 ;
   virtual const char *name() { return "-" ; }
   virtual Item_func::Functype type() { return Item_func::NEG_FUNC ; }

protected:
   bool                    negated ;
} ;

class sdb_func_between : public sdb_func_neg
{
public:
   sdb_func_between( bool has_not ) ;
   virtual ~sdb_func_between() ;

   virtual int to_bson( bson::BSONObj &obj ) ;
   virtual const char *name() { return "between" ; }
   virtual Item_func::Functype type() { return Item_func::BETWEEN ; }
} ;

class sdb_func_in : public sdb_func_neg
{
public:
   sdb_func_in( bool has_not, uint args_num ) ;
   virtual ~sdb_func_in() ;

   virtual int to_bson( bson::BSONObj &obj ) ;
   virtual const char *name() { return "in" ; }
   virtual Item_func::Functype type() { return Item_func::IN_FUNC ; }
} ;

#endif
