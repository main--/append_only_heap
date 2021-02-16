MODULE_big = append_only_heap
OBJS = append_only_heap.o

EXTENSION = append_only_heap
DATA = append_only_heap--1.0.sql
PGFILEDESC = "append_only_heap access method - like heap, but always appends to the last page and never fills gaps in other pages"

REGRESS = append_only_heap

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
