\echo Use "CREATE EXTENSION append_only_heap" to load this file. \quit

CREATE FUNCTION append_only_heap_handler(internal)
RETURNS table_am_handler
AS 'MODULE_PATHNAME'
LANGUAGE C;

-- Access method
CREATE ACCESS METHOD append_only_heap TYPE TABLE HANDLER append_only_heap_handler;
COMMENT ON ACCESS METHOD append_only_heap IS 'append only heap table access method';

