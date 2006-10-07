--
--  Note  for Mysql 4.x and 5.0 users
-- 
--  Though Mysql<5.1 supports the specification 'ON CASCADE DELETE' for CREATE TABLE
--  this is only for sql code portability: deleting a row in rivet_session doesn't 
--  imply Mysql remove the rows in rivet_session_cache matching the same session_id.
--  This table gets progressively cluttered with rows referring to sessions that are long dead.

--  This command can be used to clean up 'rivet_session_cache'

delete rivet_session_cache from rivet_session_cache left join rivet_session as t2 using(session_id) where t2.session_id is null;

--  You may subclass 'Session' in order to specialize the method do_garbage_collection and 
--  exec this command after the superclass method has run.

--  Starting with version 5.1 the 'on delete cascade' should be working (but I haven't tried)
--  and also triggers can be used to achive the same goal

--  2 Oct 2006 



