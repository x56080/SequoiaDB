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
 */
/**
 * SequoiaDB Driver for Java.
 * @package com.sequoiadb.base;
 * @author Jacky Zhang
 */
package com.sequoiadb.base;

import org.bson.BSONObject;

import java.util.HashMap;
import java.util.Map;

/**
 * Database operation rules.
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
     * Normally, query return bson object,
     *        when this flag is added, query return binary data stream.
     * @memberof FLG_QUERY_STRINGOUT 0x00000001
     */
    public static final int FLG_QUERY_STRINGOUT = 0x00000001;

    /**
     * Force to use specified hint to query,
     *        if database have no index assigned by the hint, fail to query.
     * @memberof FLG_QUERY_FORCE_HINT 0x00000080
     */
    public static final int FLG_QUERY_FORCE_HINT = 0x00000080;

    /**
     * Enable parallel sub query, each sub query will finish scanning different part of the data.
     * @memberof FLG_QUERY_PARALLED 0x00000100
     */
    public static final int FLG_QUERY_PARALLED = 0x00000100;

    /**
     * In general, query won't return data until cursor gets from database,
     *         when add this flag, return data in query response, it will be more high-performance.
     * @memberof FLG_QUERY_WITH_RETURNDATA 0x00000200
     */
    public static final int FLG_QUERY_WITH_RETURNDATA = 0x00000200;

    /**
     * Query explain.
     * @memberof FLG_QUERY_EXPLAIN 0x00000400
     */
    static final int FLG_QUERY_EXPLAIN = 0x00000400;

    /**
     * Query and modify.
     * @memberof FLG_QUERY_MODIFY 0x00001000
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
     * Get modified rule
     * @return The modified rule BSONObject
     */
    public BSONObject getModifier() {
        return modifier;
    }

    /**
     * Set modified rule
     * @param modifier The modified rule BSONObject
     */
    public void setModifier(BSONObject modifier) {
        this.modifier = modifier;
    }

    /**
     * Get selective rule.
     * @return The selective rule BSONObject
     */
    public BSONObject getSelector() {
        return selector;
    }

    /**
     * Set selective rule.
     * @param selector The selective rule BSONObject
     */
    public void setSelector(BSONObject selector) {
        this.selector = selector;
    }

    /**
     * Get matching rule.
     * @return The matching rule BSONObject
     */
    public BSONObject getMatcher() {
        return matcher;
    }

    /**
     * Set matching rule.
     * @param matcher The matching rule BSONObject
     */
    public void setMatcher(BSONObject matcher) {
        this.matcher = matcher;
    }

    /**
     * Get ordered rule.
     * @return The ordered rule BSONObject
     */
    public BSONObject getOrderBy() {
        return orderBy;
    }

    /**
     * Set ordered rule.
     * @param orderBy The ordered rule BSONObject
     */
    public void setOrderBy(BSONObject orderBy) {
        this.orderBy = orderBy;
    }

    /**
     * Get specified access plan.
     * @return The sepecified access plan BSONObject
     */
    public BSONObject getHint() {
        return hint;
    }

    /**
     * Set sepecified access plan.
     * @param hint The sepecified access plan BSONObject
     */
    public void setHint(BSONObject hint) {
        this.hint = hint;
    }

    /**
     * Get the count of BSONObjects to skip.
     * @return The count of BSONObjects to skip
     */
    public Long getSkipRowsCount() {
        return skipRowsCount;
    }

    /**
     * Set the count of BSONObjects to skip.
     * @param skipRowsCount The count of BSONObjects to skip
     */
    public void setSkipRowsCount(Long skipRowsCount) {
        this.skipRowsCount = skipRowsCount;
    }

    /**
     * Get the count of BSONObjects to return.
     * @return The count of BSONObjects to return.
     */
    public Long getReturnRowsCount() {
        return returnRowsCount;
    }

    /**
     * Set the count of BSONObjects to return.
     * @param returnRowsCount The count of BSONObjects to return
     */
    public void setReturnRowsCount(Long returnRowsCount) {
        this.returnRowsCount = returnRowsCount;
    }

    /**
     * Get the query.
     * @return The query flag
     * @see com.sequoiadb.base.DBCollection#query
     */
    public int getFlag() {
        return flag;
    }

    /**
     * Set the query flag.
     * @param flag query flag as below:
     *  	  DBQuery.FLG_QUERY_STRINGOUT
     *        DBQuery.FLG_QUERY_FORCE_HINT
     *        DBQuery.LG_QUERY_PARALLED
     *        DBQuery.FLG_QUERY_WITH_RETURNDATA  
     * @see com.sequoiadb.base.DBCollection#query
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
