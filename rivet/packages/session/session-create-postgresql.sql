--
-- Define SQL tables for session management code (Postgresql)
--
-- $Id$
--
--

create table rivet_session(
    ip_address		    inet,
    session_start_time	timestamp,
    session_update_time	timestamp,
    session_id		    varchar,

    UNIQUE( session_id )
);

create table rivet_session_cache(
    session_id		    varchar REFERENCES rivet_session(session_id) ON DELETE CASCADE,
    package_		    varchar,
    key_                varchar,
    data                varchar,

    UNIQUE( session_id, package_, key_ )
);
create index rivet_session_cache_idx ON rivet_session_cache( session_id );

