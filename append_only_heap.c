#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>

#include "postgres.h"
#include "access/heapam.h"
#include "access/rewriteheap.h"
#include "utils/fmgroids.h"


/*
 * Ideally, we could simply define a TableAmRoutine struct here where
 * all fields directly reference the builtin heap implementation,
 * except for tuple_insert, tuple_insert_speculative (not sure if that one is even needed)
 * and multi_insert. We could then implement these three by also calling
 * the builtin implementation, but with HEAP_INSERT_SKIP_FSM set.
 *
 * Sadly, the heap_getnext errors out if it's called from an access method that uses
 * a different TableAmRoutine pointer than heap itself.
 * We hack around this by using the exact same pointer as heap (which is explicitly supported
 * for tests) and overwriting the relevant function pointers in there
 * (which is not allowed because it's const but we choose to ignore that).
 *
 * Our hook functions can then perform a similar check to determine whether they're being
 * called from heap or append_only_heap.
 */

PG_MODULE_MAGIC;
PG_FUNCTION_INFO_V1(append_only_heap_handler);

static TableAmRoutine original_fns = {0};

static void hacked_tuple_insert(Relation relation, TupleTableSlot *slot, CommandId cid,
                                int options, BulkInsertState bistate)
{
	if (relation->rd_amhandler != F_HEAP_TABLEAM_HANDLER)
	{
		options |= HEAP_INSERT_SKIP_FSM;
	}
	original_fns.tuple_insert(relation, slot, cid, options, bistate);
}

static int hack_heapam_methods(TableAmRoutine* heapam)
{
	if (heapam->tuple_insert == hacked_tuple_insert) return 0; // already hooked

	int pagesize = getpagesize();
	// page-align ptr
	uintptr_t ptr = (uintptr_t)heapam;
	ptr &= ~(uintptr_t)(pagesize - 1);
	// get length
	int pages = (sizeof(TableAmRoutine) / pagesize) + 1;
	int ret = mprotect((void*)ptr, pages * pagesize, PROT_READ | PROT_WRITE);
	if (ret != 0) return 1;

	original_fns.tuple_insert = heapam->tuple_insert;
	heapam->tuple_insert = hacked_tuple_insert;

	return 0;
}

Datum
append_only_heap_handler(PG_FUNCTION_ARGS)
{
	const TableAmRoutine* heapam = GetHeapamTableAmRoutine();
        if (hack_heapam_methods((TableAmRoutine*)heapam)) PG_RETURN_NULL();
	PG_RETURN_POINTER(heapam);
}
