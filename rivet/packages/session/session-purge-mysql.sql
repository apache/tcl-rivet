--  Note
--
--  9 May 2006
--
--  Though Mysql sql interpreter supports the specification 'ON CASCADE DELETE' for CREATE TABLE, as of
--  version 5.0 this is only for sql code portability: deleting a row in rivet_session doesn't 
--  imply Mysql removes the row in rivet_session_cache sharing the same session_id.
--  This table gets progressively cluttered with rows referring to sessions that are long dead.
--  
--  As a possible workaround the programmer may build a cron procedure that runs the following
--  sql statement.

delete rivet_session_cache from rivet_session_cache  left join rivet_session as t2 using(session_id) where t2.session_id is null;

--  I don't see any cleaner approach unless the code for the garbage collection in the Session class
--  is modified for sake of portability. (Massimo Manghi)


