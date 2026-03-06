package com.sequoiadb.sdbschedule.utils;

import com.sequoiadb.testcommon.SdbTestBase;
import io.restassured.response.Response;
import org.bson.BSONObject;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class BusinessApiFactory {

    private static List< String > nodeAddrList = null;

    private static String api( String path ) {
        return "/api" + path;
    }

    private static String baseUrl() {
        return getBaseUrlRandom();
    }

    private static String getBaseUrlRandom() {
        if ( nodeAddrList == null ) {
            fetchBaseUrl();
        }
        int size = nodeAddrList.size();
        int index = ( int ) ( Math.random() * size );
        return nodeAddrList.get( index );
    }

    private static synchronized void fetchBaseUrl() {
        if ( nodeAddrList != null ) {
            return;
        }
        List< String > nodes = SdbTestBase.getSdbScheduleNodes();
        if ( nodes == null ) {
            throw new RuntimeException( "Get SdbSchedule nodes failed" );
        }
        nodeAddrList = new ArrayList<>();
        for ( String node : nodes ) {
            nodeAddrList.add( "http://" + node );
        }
    }

    // ================= GlobalConf =================
    public static class GlobalConf {

        public static Response list( long skip, long limit ) {
            Map< String, Object > params = new HashMap< String, Object >();
            params.put( "skip", skip );
            params.put( "limit", limit );
            return HttpClient.get( baseUrl(), api( "/v1/global_conf" ),
                    params );
        }

        public static Response list( long skip, long limit, String filter,
                String orderBy ) {
            Map< String, Object > params = new HashMap< String, Object >();
            params.put( "skip", skip );
            params.put( "limit", limit );
            params.put( "filter", filter );
            params.put( "orderby", orderBy );
            return HttpClient.get( baseUrl(), api( "/v1/global_conf" ),
                    params );
        }

        public static Response update( String key, String value ) {
            Map< String, Object > params = new HashMap< String, Object >();
            params.put( "conf_key", key );
            params.put( "conf_value", value );
            return HttpClient.put( baseUrl(), api( "/v1/global_conf" ),
                    params );
        }
    }

    // ================= Node =================
    public static class Node {

        public static Response list( long skip, long limit ) {
            Map< String, Object > params = new HashMap< String, Object >();
            params.put( "skip", skip );
            params.put( "limit", limit );
            return HttpClient.get( baseUrl(), api( "/v1/nodes" ), params );
        }

        public static Response list( long skip, long limit, String filter,
                String orderBy ) {
            Map< String, Object > params = new HashMap< String, Object >();
            params.put( "skip", skip );
            params.put( "limit", limit );
            params.put( "filter", filter );
            params.put( "orderby", orderBy );
            return HttpClient.get( baseUrl(), api( "/v1/nodes" ), params );
        }
    }

    // ================= Site =================
    public static class Site {

        public static Response list( long skip, long limit ) {
            Map< String, Object > params = new HashMap< String, Object >();
            params.put( "skip", skip );
            params.put( "limit", limit );
            return HttpClient.get( baseUrl(), api( "/v1/sites" ), params );
        }

        public static Response list( long skip, long limit, String filter,
                String orderBy ) {
            Map< String, Object > params = new HashMap< String, Object >();
            params.put( "skip", skip );
            params.put( "limit", limit );
            params.put( "filter", filter );
            params.put( "orderby", orderBy );
            return HttpClient.get( baseUrl(), api( "/v1/sites" ), params );
        }
    }

    // ================= Schedule =================
    public static class Schedule {

        public static Response create( Object body ) {
            return HttpClient.post( baseUrl(), api( "/v1/schedules" ), body );
        }

        public static Response update( String scheduleId, Object body ) {
            return HttpClient.put( baseUrl(),
                    api( "/v1/schedules/" + scheduleId ), body );
        }

        public static Response switchEnable( String scheduleId,
                boolean enable ) {
            Map< String, Object > params = new HashMap< String, Object >();
            params.put( "enable", enable );
            return HttpClient.put( baseUrl(),
                    api( "/v1/schedules/" + scheduleId + "/switch" ), params );
        }

        public static Response delete( String scheduleId ) {
            return HttpClient.delete( baseUrl(),
                    api( "/v1/schedules/" + scheduleId ) );
        }

        public static Response list( long skip, long limit, BSONObject filter,
                BSONObject orderBy ) {
            Map< String, Object > params = new HashMap< String, Object >();
            params.put( "skip", skip );
            params.put( "limit", limit );
            if ( filter != null ) {
                params.put( "filter", filter.toString() );
            }
            if ( orderBy != null ) {
                params.put( "orderby", orderBy.toString() );
            }
            return HttpClient.get( baseUrl(), api( "/v1/schedules" ), params );
        }

        public static Response get( String scheduleId ) {
            return HttpClient.get( baseUrl(),
                    api( "/v1/schedules/" + scheduleId ) );
        }

        public static Response listTasks( String scheduleId, long skip,
                long limit, BSONObject filter, BSONObject orderBy ) {
            Map< String, Object > params = new HashMap< String, Object >();
            params.put( "schedule_id", scheduleId );
            params.put( "skip", skip );
            params.put( "limit", limit );
            if ( filter != null ) {
                params.put( "filter", filter.toString() );
            }
            if ( orderBy != null ) {
                params.put( "orderby", orderBy.toString() );
            }
            return HttpClient.get( baseUrl(), api( "/v1/schedules/tasks" ),
                    params );
        }

        public static Response previewCSCLMatch( Object body ) {
            return HttpClient.post( baseUrl(),
                    api( "/v1/schedules/previewCSCLMatch" ), body );
        }
    }

    // ================= Task =================
    public static class Task {

        public static Response progress( String taskId ) {
            return HttpClient.get( baseUrl(),
                    api( "/v1/tasks/" + taskId + "/progress" ) );
        }

        public static Response stop( String taskId ) {
            return HttpClient.post( baseUrl(),
                    api( "/v1/tasks/" + taskId + "/stop" ), ( Object ) null );
        }
    }

}
