/**
 * Copyright (C) 2018 SequoiaDB Inc.
 * <p>
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * <p>
 * http://www.apache.org/licenses/LICENSE-2.0
 * <p>
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * @package com.sequoiadb.base;
 * @brief SequoiaDB Driver for Java
 * @author Jacky Zhang
 */
/**
 * @package com.sequoiadb.base;
 * @brief SequoiaDB Driver for Java
 * @author Jacky Zhang
 */
package com.sequoiadb.base;

import org.bson.BSONObject;

import java.util.HashMap;
import java.util.Map;

/**
 * @class DBQuery
 * @brief Database operation rules.
 */
public class DBQuery {
    private BSONObject matcher;
    private BSONObject selector;
    private BSONObject orderBy;
    private BSONObject hint;
    private BSONObject modifier;
    private Long skipRowsCount;
    private Long returnRowsCount;
    private int flag;

    /**
     * @memberof FLG_QUERY_STRINGOUT 0x00000001
     * @brief Normally, query return bson object,
     *        when this flag is added, query return binary data stream
     */
    public static final int FLG_QUERY_STRINGOUT = 0x00000001;

    /**
     * @memberof FLG_QUERY_FORCE_HINT 0x00000080
     * @brief Force to use specified hint to query,
     *        if database have no index assigned by the hint, fail to query.
     */
    public static final int FLG_QUERY_FORCE_HINT = 0x00000080;

    /**
     * @memberof FLG_QUERY_PARALLED 0x00000100
     * @brief Enable parallel sub query, each sub query will finish scanning diffent part of the data.
     */
    public static final int FLG_QUERY_PARALLED = 0x00000100;

    /**
     * @memberof FLG_QUERY_WITH_RETURNDATA 0x00000200
     * @brief In general, query won't return data until cursor gets from database, when add this flag, return data in query response, it will be more high-performance.
     */
    public static final int FLG_QUERY_WITH_RETURNDATA = 0x00000200;

    /**
     * @memberof FLG_QUERY_EXPLAIN 0x00000400
     * @brief Query explain.
     */
    static final int FLG_QUERY_EXPLAIN = 0x00000400;

    /**
     * @memberof FLG_QUERY_MODIFY 0x00001000
     * @brief Query and modify.
     */
    static final int FLG_QUERY_MODIFY = 0x00001000;

    // [ [ oldFlag, newFlag ], ... ]
    private final static int[][] flagsMap = new int[0][2];

    static {
        // add mapping flags as below, if necessary:
        //flagsMap[0][0] = FLG_QUERY_STRINGOUT;
        //flagsMap[0][1] = NEW_FLG_QUERY_STRINGOUT;
    }

    public DBQuery() {
        matcher = null;
        selector = null;
        orderBy = null;
        hint = null;
        modifier = null;
        skipRowsCount = 0L;
        returnRowsCount = -1L;
        flag = 0;
    }

    /**
     * @fn BSONObject getModifier()
     * @brief Get modified rule
     * @return The modified rule BSONObject
     */
    public BSONObject getModifier() {
        return modifier;
    }

    /**
     * @fn void setModifier(BSONObject modifier)
     * @brief Set modified rule
     * @param Modifier The modified rule BSONObject
     */
    public void setModifier(BSONObject modifier) {
        this.modifier = modifier;
    }

    /**
     * @fn BSONObject getSelector()
     * @brief Get selective rule
     * @return The selective rule BSONObject
     */
    public BSONObject getSelector() {
        return selector;
    }

    /**
     * @fn void setSelector(BSONObject selector)
     * @brief Set selective rule
     * @param Selector The selective rule BSONObject
     */
    public void setSelector(BSONObject selector) {
        this.selector = selector;
    }

    /**
     * @fn BSONObject getMatcher()
     * @brief Get matching rule
     * @return The matching rule BSONObject
     */
    public BSONObject getMatcher() {
        return matcher;
    }

    /**
     * @fn void setMatcher(BSONObject matcher)
     * @brief Set matching rule
     * @param Matcher The matching rule BSONObject
     */
    public void setMatcher(BSONObject matcher) {
        this.matcher = matcher;
    }

    /**
     * @fn BSONObject getOrderBy()
     * @brief Get ordered rule
     * @return The ordered rule BSONObject
     */
    public BSONObject getOrderBy() {
        return orderBy;
    }

    /**
     * @fn void setOrderBy(BSONObject orderBy)
     * @brief Set ordered rule
     * @param OrderBy The ordered rule BSONObject
     */
    public void setOrderBy(BSONObject orderBy) {
        this.orderBy = orderBy;
    }

    /**
     * @fn BSONObject getHint()
     * @brief Get sepecified access plan
     * @return The sepecified access plan BSONObject
     */
    public BSONObject getHint() {
        return hint;
    }

    /**
     * @fn void setHint(BSONObject hint)
     * @brief Set sepecified access plan
     * @param Hint The sepecified access plan BSONObject
     */
    public void setHint(BSONObject hint) {
        this.hint = hint;
    }

    /**
     * @fn Long getSkipRowsCount()
     * @brief Get the count of BSONObjects to skip
     * @return The count of BSONObjects to skip
     */
    public Long getSkipRowsCount() {
        return skipRowsCount;
    }

    /**
     * @fn void setSkipRowsCount(Long skipRowsCount)
     * @brief Set the count of BSONObjects to skip
     * @param SkipRowsCount The count of BSONObjects to skip
     */
    public void setSkipRowsCount(Long skipRowsCount) {
        this.skipRowsCount = skipRowsCount;
    }

    /**
     * @fn Long getReturnRowsCount()
     * @brief Get the count of BSONObjects to return
     * @return The count of BSONObjects to return
     */
    public Long getReturnRowsCount() {
        return returnRowsCount;
    }

    /**
     * @fn void setReturnRowsCount(Long returnRowsCount)
     * @brief Set the count of BSONObjects to return
     * @param ReturnRowsCount The count of BSONObjects to return
     */
    public void setReturnRowsCount(Long returnRowsCount) {
        this.returnRowsCount = returnRowsCount;
    }

    /**
     * @fn int getFlag()
     * @brief Get the query
     * @return The query flag
     * @see com.sequoiadb.base.DBCollection.query
     */
    public int getFlag() {
        return flag;
    }

    /**
     * @fn void setFlag(int flag)
     * @brief Set the query flag
     * @param The query flag as below:
     *  	  DBQuery.FLG_QUERY_STRINGOUT
     *        DBQuery.FLG_QUERY_FORCE_HINT
     *        DBQuery.LG_QUERY_PARALLED
     *        DBQuery.FLG_QUERY_WITH_RETURNDATA  
     * @see com.sequoiadb.base.DBCollection.query
     */
    public void setFlag(int flag) {
        this.flag = flag;
    }

    static int regulateFlags(final int flags) {
        if (flagsMap.length > 0) {
            int newFlags = flags;
            for (int[] flagMap : flagsMap) {
                if (flagMap[0] != flagMap[1] && (flags & flagMap[0]) != 0) {
                    newFlags &= ~flagMap[0];
                    newFlags |= flagMap[1];
                }
            }
            return newFlags;
        } else {
            return flags;
        }
    }


    static int eraseSingleFlag(final int flags, int erasedFlag) {
        int newFlags = flags;
        if ((newFlags & erasedFlag) != 0) {
            newFlags &= ~erasedFlag;
        }
        return newFlags;
    }

}
