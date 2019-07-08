/*
   Copyright 2018-2019 Banco Bilbao Vizcaya Argentaria, S.A.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

/*

    C bindings for missing rocksdb features

*/

#pragma once

#ifndef ROCKSDB_EXTENDED_H
#define ROCKSDB_EXTENDED_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rocksdb/c.h"
#include <stdint.h>

/* Exported types */

typedef struct rocksdb_statistics_t rocksdb_statistics_t;
typedef struct rocksdb_histogram_data_t rocksdb_histogram_data_t;


/* DB operations */ 

extern rocksdb_statistics_t* rocksdb_create_statistics();

/* returns 0 on success, -1 otherwise */
extern int rocksdb_property_int_cf(
    rocksdb_t* db, rocksdb_column_family_handle_t* column_family,
    const char* propname, uint64_t *out_val);

/* Options */

extern void rocksdb_options_set_atomic_flush(
    rocksdb_options_t*, unsigned char);

extern void rocksdb_options_set_statistics(
    rocksdb_options_t* opts, 
    rocksdb_statistics_t* stats);

/* Cache */

extern rocksdb_cache_t* rocksdb_cache_create_lru_with_ratio(
    size_t capacity, double hi_pri_pool_ratio);

/* Slice Transform */

extern void rocksdb_destruct_handler(void* state);

extern rocksdb_slicetransform_t* rocksdb_slicetransform_create_ext(uintptr_t idx);

/* Backup */
extern void rocksdb_backup_engine_create_new_backup_with_metadata(
    rocksdb_backup_engine_t* be, rocksdb_t* db, char* app_metadata, char** errptr);

extern char* rocksdb_backup_engine_info_metadata(
    const rocksdb_backup_engine_info_t* info, int index);

extern void rocksdb_backup_engine_restore_db_from_backup(
    rocksdb_backup_engine_t* be, uint32_t backupID, const char* db_dir, const char* wal_dir,
    const rocksdb_restore_options_t* restore_options, char** errptr);

/* Statistics */

typedef enum {
    // total block cache misses
    // REQUIRES: BLOCK_CACHE_MISS == BLOCK_CACHE_INDEX_MISS +
    //                               BLOCK_CACHE_FILTER_MISS +
    //                               BLOCK_CACHE_DATA_MISS;
    BLOCK_CACHE_MISS = 0,
    // total block cache hit
    // REQUIRES: BLOCK_CACHE_HIT == BLOCK_CACHE_INDEX_HIT +
    //                              BLOCK_CACHE_FILTER_HIT +
    //                              BLOCK_CACHE_DATA_HIT;
    BLOCK_CACHE_HIT,
    // # of blocks added to block cache.
    BLOCK_CACHE_ADD,
    // # of failures when adding blocks to block cache.
    BLOCK_CACHE_ADD_FAILURES,
    // # of times cache miss when accessing index block from block cache.
    BLOCK_CACHE_INDEX_MISS,
    // # of times cache hit when accessing index block from block cache.
    BLOCK_CACHE_INDEX_HIT,
    // # of index blocks added to block cache.
    BLOCK_CACHE_INDEX_ADD,
    // # of bytes of index blocks inserted into cache
    BLOCK_CACHE_INDEX_BYTES_INSERT,
    // # of bytes of index block erased from cache
    BLOCK_CACHE_INDEX_BYTES_EVICT,
    // # of times cache miss when accessing filter block from block cache.
    BLOCK_CACHE_FILTER_MISS,
    // # of times cache hit when accessing filter block from block cache.
    BLOCK_CACHE_FILTER_HIT,
    // # of filter blocks added to block cache.
    BLOCK_CACHE_FILTER_ADD,
    // # of bytes of bloom filter blocks inserted into cache
    BLOCK_CACHE_FILTER_BYTES_INSERT,
    // # of bytes of bloom filter block erased from cache
    BLOCK_CACHE_FILTER_BYTES_EVICT,
    // # of times cache miss when accessing data block from block cache.
    BLOCK_CACHE_DATA_MISS,
    // # of times cache hit when accessing data block from block cache.
    BLOCK_CACHE_DATA_HIT,
    // # of data blocks added to block cache.
    BLOCK_CACHE_DATA_ADD,
    // # of bytes of data blocks inserted into cache
    BLOCK_CACHE_DATA_BYTES_INSERT,
    // # of bytes read from cache.
    BLOCK_CACHE_BYTES_READ,
    // # of bytes written into cache.
    BLOCK_CACHE_BYTES_WRITE,

    // # of times bloom filter has avoided file reads, i.e., negatives.
    BLOOM_FILTER_USEFUL,
    // # of times bloom FullFilter has not avoided the reads.
    BLOOM_FILTER_FULL_POSITIVE,
    // # of times bloom FullFilter has not avoided the reads and data actually
    // exist.
    BLOOM_FILTER_FULL_TRUE_POSITIVE,

    // # persistent cache hit
    PERSISTENT_CACHE_HIT,
    // # persistent cache miss
    PERSISTENT_CACHE_MISS,

    // # total simulation block cache hits
    SIM_BLOCK_CACHE_HIT,
    // # total simulation block cache misses
    SIM_BLOCK_CACHE_MISS,

    // # of memtable hits.
    MEMTABLE_HIT,
    // # of memtable misses.
    MEMTABLE_MISS,

    // # of Get() queries served by L0
    GET_HIT_L0,
    // # of Get() queries served by L1
    GET_HIT_L1,
    // # of Get() queries served by L2 and up
    GET_HIT_L2_AND_UP,

    /**
     * COMPACTION_KEY_DROP_* count the reasons for key drop during compaction
     * There are 4 reasons currently.
     */
    COMPACTION_KEY_DROP_NEWER_ENTRY,  // key was written with a newer value.
                                        // Also includes keys dropped for range del.
    COMPACTION_KEY_DROP_OBSOLETE,     // The key is obsolete.
    COMPACTION_KEY_DROP_RANGE_DEL,    // key was covered by a range tombstone.
    COMPACTION_KEY_DROP_USER,  // user compaction function has dropped the key.
    COMPACTION_RANGE_DEL_DROP_OBSOLETE,  // all keys in range were deleted.
    // Deletions obsoleted before bottom level due to file gap optimization.
    COMPACTION_OPTIMIZED_DEL_DROP_OBSOLETE,
    // If a compaction was cancelled in sfm to prevent ENOSPC
    COMPACTION_CANCELLED,

    // Number of keys written to the database via the Put and Write call's
    NUMBER_KEYS_WRITTEN,
    // Number of Keys read,
    NUMBER_KEYS_READ,
    // Number keys updated, if inplace update is enabled
    NUMBER_KEYS_UPDATED,
    // The number of uncompressed bytes issued by DB::Put(), DB::Delete(),
    // DB::Merge(), and DB::Write().
    BYTES_WRITTEN,
    // The number of uncompressed bytes read from DB::Get().  It could be
    // either from memtables, cache, or table files.
    // For the number of logical bytes read from DB::MultiGet(),
    // please use NUMBER_MULTIGET_BYTES_READ.
    BYTES_READ,
    // The number of calls to seek/next/prev
    NUMBER_DB_SEEK,
    NUMBER_DB_NEXT,
    NUMBER_DB_PREV,
    // The number of calls to seek/next/prev that returned data
    NUMBER_DB_SEEK_FOUND,
    NUMBER_DB_NEXT_FOUND,
    NUMBER_DB_PREV_FOUND,
    // The number of uncompressed bytes read from an iterator.
    // Includes size of key and value.
    ITER_BYTES_READ,
    NO_FILE_CLOSES,
    NO_FILE_OPENS,
    NO_FILE_ERRORS,
    // DEPRECATED Time system had to wait to do LO-L1 compactions
    STALL_L0_SLOWDOWN_MICROS,
    // DEPRECATED Time system had to wait to move memtable to L1.
    STALL_MEMTABLE_COMPACTION_MICROS,
    // DEPRECATED write throttle because of too many files in L0
    STALL_L0_NUM_FILES_MICROS,
    // Writer has to wait for compaction or flush to finish.
    STALL_MICROS,
    // The wait time for db mutex.
    // Disabled by default. To enable it set stats level to kAll
    DB_MUTEX_WAIT_MICROS,
    RATE_LIMIT_DELAY_MILLIS,
    // DEPRECATED number of iterators currently open
    NO_ITERATORS,

    // Number of MultiGet calls, keys read, and bytes read
    NUMBER_MULTIGET_CALLS,
    NUMBER_MULTIGET_KEYS_READ,
    NUMBER_MULTIGET_BYTES_READ,

    // Number of deletes records that were not required to be
    // written to storage because key does not exist
    NUMBER_FILTERED_DELETES,
    NUMBER_MERGE_FAILURES,

    // number of times bloom was checked before creating iterator on a
    // file, and the number of times the check was useful in avoiding
    // iterator creation (and thus likely IOPs).
    BLOOM_FILTER_PREFIX_CHECKED,
    BLOOM_FILTER_PREFIX_USEFUL,

    // Number of times we had to reseek inside an iteration to skip
    // over large number of keys with same userkey.
    NUMBER_OF_RESEEKS_IN_ITERATION,

    // Record the number of calls to GetUpadtesSince. Useful to keep track of
    // transaction log iterator refreshes
    GET_UPDATES_SINCE_CALLS,
    BLOCK_CACHE_COMPRESSED_MISS,  // miss in the compressed block cache
    BLOCK_CACHE_COMPRESSED_HIT,   // hit in the compressed block cache
    // Number of blocks added to compressed block cache
    BLOCK_CACHE_COMPRESSED_ADD,
    // Number of failures when adding blocks to compressed block cache
    BLOCK_CACHE_COMPRESSED_ADD_FAILURES,
    WAL_FILE_SYNCED,  // Number of times WAL sync is done
    WAL_FILE_BYTES,   // Number of bytes written to WAL

    // Writes can be processed by requesting thread or by the thread at the
    // head of the writers queue.
    WRITE_DONE_BY_SELF,
    WRITE_DONE_BY_OTHER,  // Equivalent to writes done for others
    WRITE_TIMEDOUT,       // Number of writes ending up with timed-out.
    WRITE_WITH_WAL,       // Number of Write calls that request WAL
    COMPACT_READ_BYTES,   // Bytes read during compaction
    COMPACT_WRITE_BYTES,  // Bytes written during compaction
    FLUSH_WRITE_BYTES,    // Bytes written during flush

    // Number of table's properties loaded directly from file, without creating
    // table reader object.
    NUMBER_DIRECT_LOAD_TABLE_PROPERTIES,
    NUMBER_SUPERVERSION_ACQUIRES,
    NUMBER_SUPERVERSION_RELEASES,
    NUMBER_SUPERVERSION_CLEANUPS,

    // # of compressions/decompressions executed
    NUMBER_BLOCK_COMPRESSED,
    NUMBER_BLOCK_DECOMPRESSED,

    NUMBER_BLOCK_NOT_COMPRESSED,
    MERGE_OPERATION_TOTAL_TIME,
    FILTER_OPERATION_TOTAL_TIME,

    // Row cache.
    ROW_CACHE_HIT,
    ROW_CACHE_MISS,

    // Read amplification statistics.
    // Read amplification can be calculated using this formula
    // (READ_AMP_TOTAL_READ_BYTES / READ_AMP_ESTIMATE_USEFUL_BYTES)
    //
    // REQUIRES: ReadOptions::read_amp_bytes_per_bit to be enabled
    READ_AMP_ESTIMATE_USEFUL_BYTES,  // Estimate of total bytes actually used.
    READ_AMP_TOTAL_READ_BYTES,       // Total size of loaded data blocks.

    // Number of refill intervals where rate limiter's bytes are fully consumed.
    NUMBER_RATE_LIMITER_DRAINS,

    // Number of internal keys skipped by Iterator
    NUMBER_ITER_SKIP,

    // BlobDB specific stats
    // # of Put/PutTTL/PutUntil to BlobDB.
    BLOB_DB_NUM_PUT,
    // # of Write to BlobDB.
    BLOB_DB_NUM_WRITE,
    // # of Get to BlobDB.
    BLOB_DB_NUM_GET,
    // # of MultiGet to BlobDB.
    BLOB_DB_NUM_MULTIGET,
    // # of Seek/SeekToFirst/SeekToLast/SeekForPrev to BlobDB iterator.
    BLOB_DB_NUM_SEEK,
    // # of Next to BlobDB iterator.
    BLOB_DB_NUM_NEXT,
    // # of Prev to BlobDB iterator.
    BLOB_DB_NUM_PREV,
    // # of keys written to BlobDB.
    BLOB_DB_NUM_KEYS_WRITTEN,
    // # of keys read from BlobDB.
    BLOB_DB_NUM_KEYS_READ,
    // # of bytes (key + value) written to BlobDB.
    BLOB_DB_BYTES_WRITTEN,
    // # of bytes (keys + value) read from BlobDB.
    BLOB_DB_BYTES_READ,
    // # of keys written by BlobDB as non-TTL inlined value.
    BLOB_DB_WRITE_INLINED,
    // # of keys written by BlobDB as TTL inlined value.
    BLOB_DB_WRITE_INLINED_TTL,
    // # of keys written by BlobDB as non-TTL blob value.
    BLOB_DB_WRITE_BLOB,
    // # of keys written by BlobDB as TTL blob value.
    BLOB_DB_WRITE_BLOB_TTL,
    // # of bytes written to blob file.
    BLOB_DB_BLOB_FILE_BYTES_WRITTEN,
    // # of bytes read from blob file.
    BLOB_DB_BLOB_FILE_BYTES_READ,
    // # of times a blob files being synced.
    BLOB_DB_BLOB_FILE_SYNCED,
    // # of blob index evicted from base DB by BlobDB compaction filter because
    // of expiration.
    BLOB_DB_BLOB_INDEX_EXPIRED_COUNT,
    // size of blob index evicted from base DB by BlobDB compaction filter
    // because of expiration.
    BLOB_DB_BLOB_INDEX_EXPIRED_SIZE,
    // # of blob index evicted from base DB by BlobDB compaction filter because
    // of corresponding file deleted.
    BLOB_DB_BLOB_INDEX_EVICTED_COUNT,
    // size of blob index evicted from base DB by BlobDB compaction filter
    // because of corresponding file deleted.
    BLOB_DB_BLOB_INDEX_EVICTED_SIZE,
    // # of blob files being garbage collected.
    BLOB_DB_GC_NUM_FILES,
    // # of blob files generated by garbage collection.
    BLOB_DB_GC_NUM_NEW_FILES,
    // # of BlobDB garbage collection failures.
    BLOB_DB_GC_FAILURES,
    // # of keys drop by BlobDB garbage collection because they had been
    // overwritten.
    BLOB_DB_GC_NUM_KEYS_OVERWRITTEN,
    // # of keys drop by BlobDB garbage collection because of expiration.
    BLOB_DB_GC_NUM_KEYS_EXPIRED,
    // # of keys relocated to new blob file by garbage collection.
    BLOB_DB_GC_NUM_KEYS_RELOCATED,
    // # of bytes drop by BlobDB garbage collection because they had been
    // overwritten.
    BLOB_DB_GC_BYTES_OVERWRITTEN,
    // # of bytes drop by BlobDB garbage collection because of expiration.
    BLOB_DB_GC_BYTES_EXPIRED,
    // # of bytes relocated to new blob file by garbage collection.
    BLOB_DB_GC_BYTES_RELOCATED,
    // # of blob files evicted because of BlobDB is full.
    BLOB_DB_FIFO_NUM_FILES_EVICTED,
    // # of keys in the blob files evicted because of BlobDB is full.
    BLOB_DB_FIFO_NUM_KEYS_EVICTED,
    // # of bytes in the blob files evicted because of BlobDB is full.
    BLOB_DB_FIFO_BYTES_EVICTED,

    // These counters indicate a performance issue in WritePrepared transactions.
    // We should not seem them ticking them much.
    // # of times prepare_mutex_ is acquired in the fast path.
    TXN_PREPARE_MUTEX_OVERHEAD,
    // # of times old_commit_map_mutex_ is acquired in the fast path.
    TXN_OLD_COMMIT_MAP_MUTEX_OVERHEAD,
    // # of times we checked a batch for duplicate keys.
    TXN_DUPLICATE_KEY_OVERHEAD,
    // # of times snapshot_mutex_ is acquired in the fast path.
    TXN_SNAPSHOT_MUTEX_OVERHEAD,

    // Number of keys actually found in MultiGet calls (vs number requested by
    // caller)
    // NUMBER_MULTIGET_KEYS_READ gives the number requested by caller
    NUMBER_MULTIGET_KEYS_FOUND,

    NO_ITERATOR_CREATED,  // number of iterators created
    NO_ITERATOR_DELETED,  // number of iterators deleted

    BLOCK_CACHE_COMPRESSION_DICT_MISS,
    BLOCK_CACHE_COMPRESSION_DICT_HIT,
    BLOCK_CACHE_COMPRESSION_DICT_ADD,
    BLOCK_CACHE_COMPRESSION_DICT_BYTES_INSERT,
    BLOCK_CACHE_COMPRESSION_DICT_BYTES_EVICT,
    TICKER_ENUM_MAX
} rocksdb_tickers_t;

typedef enum {
    /**
     * Keep adding histogram's here.
     * Any histogram should have value less than HISTOGRAM_ENUM_MAX
     * Add a new Histogram by assigning it the current value of HISTOGRAM_ENUM_MAX
     * Add a string representation in HistogramsNameMap below
     * And increment HISTOGRAM_ENUM_MAX
     * Add a corresponding enum value to HistogramType.java in the java API
     */
    DB_GET = 0,
    DB_WRITE,
    COMPACTION_TIME,
    COMPACTION_CPU_TIME,
    SUBCOMPACTION_SETUP_TIME,
    TABLE_SYNC_MICROS,
    COMPACTION_OUTFILE_SYNC_MICROS,
    WAL_FILE_SYNC_MICROS,
    MANIFEST_FILE_SYNC_MICROS,
    // TIME SPENT IN IO DURING TABLE OPEN
    TABLE_OPEN_IO_MICROS,
    DB_MULTIGET,
    READ_BLOCK_COMPACTION_MICROS,
    READ_BLOCK_GET_MICROS,
    WRITE_RAW_BLOCK_MICROS,
    STALL_L0_SLOWDOWN_COUNT,
    STALL_MEMTABLE_COMPACTION_COUNT,
    STALL_L0_NUM_FILES_COUNT,
    HARD_RATE_LIMIT_DELAY_COUNT,
    SOFT_RATE_LIMIT_DELAY_COUNT,
    NUM_FILES_IN_SINGLE_COMPACTION,
    DB_SEEK,
    WRITE_STALL,
    SST_READ_MICROS,
    // The number of subcompactions actually scheduled during a compaction
    NUM_SUBCOMPACTIONS_SCHEDULED,
    // Value size distribution in each operation
    BYTES_PER_READ,
    BYTES_PER_WRITE,
    BYTES_PER_MULTIGET,

    // number of bytes compressed/decompressed
    // number of bytes is when uncompressed; i.e. before/after respectively
    BYTES_COMPRESSED,
    BYTES_DECOMPRESSED,
    COMPRESSION_TIMES_NANOS,
    DECOMPRESSION_TIMES_NANOS,
    // Number of merge operands passed to the merge operator in user read
    // requests.
    READ_NUM_MERGE_OPERANDS,

    // BlobDB specific stats
    // Size of keys written to BlobDB.
    BLOB_DB_KEY_SIZE,
    // Size of values written to BlobDB.
    BLOB_DB_VALUE_SIZE,
    // BlobDB Put/PutWithTTL/PutUntil/Write latency.
    BLOB_DB_WRITE_MICROS,
    // BlobDB Get lagency.
    BLOB_DB_GET_MICROS,
    // BlobDB MultiGet latency.
    BLOB_DB_MULTIGET_MICROS,
    // BlobDB Seek/SeekToFirst/SeekToLast/SeekForPrev latency.
    BLOB_DB_SEEK_MICROS,
    // BlobDB Next latency.
    BLOB_DB_NEXT_MICROS,
    // BlobDB Prev latency.
    BLOB_DB_PREV_MICROS,
    // Blob file write latency.
    BLOB_DB_BLOB_FILE_WRITE_MICROS,
    // Blob file read latency.
    BLOB_DB_BLOB_FILE_READ_MICROS,
    // Blob file sync latency.
    BLOB_DB_BLOB_FILE_SYNC_MICROS,
    // BlobDB garbage collection time.
    BLOB_DB_GC_MICROS,
    // BlobDB compression time.
    BLOB_DB_COMPRESSION_MICROS,
    // BlobDB decompression time.
    BLOB_DB_DECOMPRESSION_MICROS,
    // Time spent flushing memtable to disk
    FLUSH_TIME,

    HISTOGRAM_ENUM_MAX,
} rocksdb_histograms_t;

typedef enum {
    // Disable timer stats, and skip histogram stats
    EXCEPT_HISTOGRAM_OR_TIMERS = 0,
    // Skip timer stats
    EXCEPT_TIMERS,
    // Collect all stats except time inside mutex lock AND time spent on
    // compression.
    EXCEPT_DETAILED_TIMERS,
    // Collect all stats except the counters requiring to get time inside the
    // mutex lock.
    EXCEPT_TIME_FOR_MUTEX,
    // Collect all stats, including measuring duration of mutex operations.
    // If getting time is expensive on the platform to run, it can
    // reduce scalability to more threads, especially for writes.
    ALL
} rocksdb_stats_level_t;

extern rocksdb_stats_level_t rocksdb_statistics_stats_level(
    rocksdb_statistics_t* stats);

extern void rocksdb_statistics_set_stats_level(
    rocksdb_statistics_t* stats,
    rocksdb_stats_level_t level);

extern void rocksdb_statistics_reset(rocksdb_statistics_t* stats);

extern uint64_t rocksdb_statistics_get_ticker_count(
    rocksdb_statistics_t* stats, 
    rocksdb_tickers_t ticker_type);

extern uint64_t rocksdb_statistics_get_and_reset_ticker_count(
    rocksdb_statistics_t* stats, 
    rocksdb_tickers_t ticker_type);

extern void rocksdb_statistics_destroy(rocksdb_statistics_t* stats);

extern void rocksdb_statistics_histogram_data(
    const rocksdb_statistics_t* stats, 
    rocksdb_histograms_t type, 
    const rocksdb_histogram_data_t* data);

extern rocksdb_histogram_data_t* rocksdb_histogram_create_data();

extern double rocksdb_histogram_get_average(rocksdb_histogram_data_t* data);

extern double rocksdb_histogram_get_median(rocksdb_histogram_data_t* data);

extern double rocksdb_histogram_get_percentile95(rocksdb_histogram_data_t* data);

extern double rocksdb_histogram_get_percentile99(rocksdb_histogram_data_t* data);

extern double rocksdb_histogram_get_stdev(rocksdb_histogram_data_t* data);

extern double rocksdb_histogram_get_max(rocksdb_histogram_data_t* data);

extern uint64_t rocksdb_histogram_get_count(rocksdb_histogram_data_t* data);

extern uint64_t rocksdb_histogram_get_sum(rocksdb_histogram_data_t* data);

extern void rocksdb_histogram_data_destroy(rocksdb_histogram_data_t* data);

#ifdef __cplusplus
}
#endif

#endif // ROCKSDB_EXTENDED_H