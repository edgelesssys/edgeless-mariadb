/*****************************************************************************

Copyright (c) 2012, 2015, Oracle and/or its affiliates. All rights reserved.
Copyright (c) 2017, 2021, MariaDB Corporation.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; version 2 of the License.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1335 USA

*****************************************************************************/

/**************************************************//**
@file include/dict0stats.ic
Code used for calculating and manipulating table statistics.

Created Jan 23, 2012 Vasil Dimov
*******************************************************/

#include "dict0dict.h"
#include "srv0srv.h"

/*********************************************************************//**
Set the persistent statistics flag for a given table. This is set only
in the in-memory table object and is not saved on disk. It will be read
from the .frm file upon first open from MySQL after a server restart. */
UNIV_INLINE
void
dict_stats_set_persistent(
/*======================*/
	dict_table_t*	table,	/*!< in/out: table */
	ibool		ps_on,	/*!< in: persistent stats explicitly enabled */
	ibool		ps_off)	/*!< in: persistent stats explicitly disabled */
{
	/* Not allowed to have both flags set, but a CREATE or ALTER
	statement that contains "STATS_PERSISTENT=0 STATS_PERSISTENT=1" would
	end up having both set. In this case we clear the OFF flag. */
	if (ps_on && ps_off) {
		ps_off = FALSE;
	}

	ib_uint32_t	stat_persistent = 0;

	if (ps_on) {
		stat_persistent |= DICT_STATS_PERSISTENT_ON;
	}

	if (ps_off) {
		stat_persistent |= DICT_STATS_PERSISTENT_OFF;
	}

	/* we rely on this assignment to be atomic */
	table->stat_persistent = stat_persistent;
}

/** @return whether persistent statistics is enabled for a given table */
UNIV_INLINE
bool
dict_stats_is_persistent_enabled(const dict_table_t* table)
{
	/* Because of the nature of this check (non-locking) it is possible
	that a table becomes:
	* PS-disabled immediately after this function has returned TRUE or
	* PS-enabled immediately after this function has returned FALSE.
	This means that it is possible that we do:
	+ dict_stats_update(DICT_STATS_RECALC_PERSISTENT) on a table that has
	  just been PS-disabled or
	+ dict_stats_update(DICT_STATS_RECALC_TRANSIENT) on a table that has
	  just been PS-enabled.
	This is acceptable. Avoiding this would mean that we would have to
	hold dict_sys.latch or stats_mutex_lock() like for accessing the
	other ::stat_ members which would be too big performance penalty,
	especially when this function is called from
	dict_stats_update_if_needed(). */

	/* we rely on this read to be atomic */
	ib_uint32_t	stat_persistent = table->stat_persistent;

	if (stat_persistent & DICT_STATS_PERSISTENT_ON) {
		ut_ad(!(stat_persistent & DICT_STATS_PERSISTENT_OFF));
		return(true);
	} else if (stat_persistent & DICT_STATS_PERSISTENT_OFF) {
		return(false);
	} else {
		return(srv_stats_persistent);
	}
}

/*********************************************************************//**
Set the auto recalc flag for a given table (only honored for a persistent
stats enabled table). The flag is set only in the in-memory table object
and is not saved in InnoDB files. It will be read from the .frm file upon
first open from MySQL after a server restart. */
UNIV_INLINE
void
dict_stats_auto_recalc_set(
/*=======================*/
	dict_table_t*	table,			/*!< in/out: table */
	ibool		auto_recalc_on,		/*!< in: explicitly enabled */
	ibool		auto_recalc_off)	/*!< in: explicitly disabled */
{
	ut_ad(!auto_recalc_on || !auto_recalc_off);

	ib_uint32_t	stats_auto_recalc = 0;

	if (auto_recalc_on) {
		stats_auto_recalc |= DICT_STATS_AUTO_RECALC_ON;
	}

	if (auto_recalc_off) {
		stats_auto_recalc |= DICT_STATS_AUTO_RECALC_OFF;
	}

	/* we rely on this assignment to be atomic */
	table->stats_auto_recalc = stats_auto_recalc;
}

/** @return whether auto recalc is enabled for a given table*/
UNIV_INLINE
bool
dict_stats_auto_recalc_is_enabled(const dict_table_t* table)
{
	/* we rely on this read to be atomic */
	ib_uint32_t	stats_auto_recalc = table->stats_auto_recalc;

	if (stats_auto_recalc & DICT_STATS_AUTO_RECALC_ON) {
		ut_ad(!(stats_auto_recalc & DICT_STATS_AUTO_RECALC_OFF));
		return(true);
	} else if (stats_auto_recalc & DICT_STATS_AUTO_RECALC_OFF) {
		return(false);
	} else {
		return(srv_stats_auto_recalc);
	}
}

/*********************************************************************//**
Initialize table's stats for the first time when opening a table. */
UNIV_INLINE
void
dict_stats_init(
/*============*/
	dict_table_t*	table)	/*!< in/out: table */
{
	ut_ad(!table->stats_mutex_is_owner());

	if (table->stat_initialized) {
		return;
	}

	dict_stats_upd_option_t	opt;

	if (dict_stats_is_persistent_enabled(table)) {
		opt = DICT_STATS_FETCH_ONLY_IF_NOT_IN_MEMORY;
	} else {
		opt = DICT_STATS_RECALC_TRANSIENT;
	}

	dict_stats_update(table, opt);
}

/*********************************************************************//**
Deinitialize table's stats after the last close of the table. This is
used to detect "FLUSH TABLE" and refresh the stats upon next open. */
UNIV_INLINE
void
dict_stats_deinit(
/*==============*/
	dict_table_t*	table)	/*!< in/out: table */
{
	ut_ad(table->stats_mutex_is_owner());
	ut_ad(table->get_ref_count() == 0);

#ifdef HAVE_valgrind
	if (!table->stat_initialized) {
		return;
	}

	MEM_UNDEFINED(&table->stat_n_rows, sizeof table->stat_n_rows);
	MEM_UNDEFINED(&table->stat_clustered_index_size,
		      sizeof table->stat_clustered_index_size);
	MEM_UNDEFINED(&table->stat_sum_of_other_index_sizes,
		      sizeof table->stat_sum_of_other_index_sizes);
	MEM_UNDEFINED(&table->stat_modified_counter,
		      sizeof table->stat_modified_counter);

	dict_index_t*   index;

	for (index = dict_table_get_first_index(table);
	     index != NULL;
	     index = dict_table_get_next_index(index)) {
		MEM_UNDEFINED(
			index->stat_n_diff_key_vals,
			index->n_uniq
			* sizeof index->stat_n_diff_key_vals[0]);
		MEM_UNDEFINED(
			index->stat_n_sample_sizes,
			index->n_uniq
			* sizeof index->stat_n_sample_sizes[0]);
		MEM_UNDEFINED(
			index->stat_n_non_null_key_vals,
			index->n_uniq
			* sizeof index->stat_n_non_null_key_vals[0]);
		MEM_UNDEFINED(
			&index->stat_index_size,
			sizeof(index->stat_index_size));
		MEM_UNDEFINED(
			&index->stat_n_leaf_pages,
			sizeof(index->stat_n_leaf_pages));
	}
#endif /* HAVE_valgrind */
	table->stat_initialized = FALSE;
}
