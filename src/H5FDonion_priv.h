/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the COPYING file, which can be found at the root of the source code       *
 * distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.  *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Onion Virtual File Driver (VFD) Internals.
 *
 * Programmer:  Jacob "Jake" Smith
 *              8 August 2020
 *
 * Purpose:    The private header file for the Onion VFD.
 *             Contains definitions and declarations used internallay and by
 *             tests.
 */

#ifndef H5FDonion_priv_H
#define H5FDonion_priv_H

#ifdef H5_HAVE_ONION_VFD

/*
 * INTERNAL MACROS AND DEFINITIONS
 */

#define H5FD__ONION_ARCHIVAL_INDEX_MAGIC 0x11111111 /* TODO */
#define H5FD__ONION_ARCHIVAL_INDEX_VERSION_CURR 1

/* Number of bytes to encode fixed-size components */
#define H5FD__ONION_ENCODED_SIZE_HEADER                 40
#define H5FD__ONION_ENCODED_SIZE_INDEX_ENTRY            20
#define H5FD__ONION_ENCODED_SIZE_RECORD_POINTER         20
#define H5FD__ONION_ENCODED_SIZE_REVISION_RECORD        76
#define H5FD__ONION_ENCODED_SIZE_WHOLE_HISTORY          20

/* Flags must align exactly one per bit, up to 24 bits */
#define H5FD__ONION_HEADER_FLAG_WRITE_LOCK              0x1
#define H5FD__ONION_HEADER_FLAG_DIVERGENT_HISTORY       0x2
#define H5FD__ONION_HEADER_FLAG_PAGE_ALIGNMENT          0x4
#define H5FD__ONION_HEADER_MAGIC                        0x433421fa /* TODO */
#define H5FD__ONION_HEADER_SIGNATURE                    "OHDH"
#define H5FD__ONION_HEADER_VERSION_CURR                 (uint8_t)1

#define H5FD__ONION_REVISION_INDEX_HASH_CHAIN_NODE_MAGIC 0x33333333 /* TODO */
#define H5FD__ONION_REVISION_INDEX_HASH_CHAIN_NODE_VERSION_CURR 1
#define H5FD__ONION_REVISION_INDEX_MAGIC                0x22222222 /* TODO */
#define H5FD__ONION_REVISION_INDEX_STARTING_SIZE_LOG2   10 /* 2^n slots */
#define H5FD__ONION_REVISION_INDEX_VERSION_CURR         (uint8_t)1

#define H5FD__ONION_REVISION_RECORD_MAGIC               0x54672381 /* TODO */
#define H5FD__ONION_REVISION_RECORD_SIGNATURE           "ORRS"
#define H5FD__ONION_REVISION_RECORD_VERSION_CURR        (uint8_t)1

#define H5FD__ONION_WHOLE_HISTORY_MAGIC                 0xb38a0921 /* TODO */
#define H5FD__ONION_WHOLE_HISTORY_SIGNATURE             "OWHS"
#define H5FD__ONION_WHOLE_HISTORY_VERSION_CURR          (uint8_t)1

/*
 * INTERNAL STRUCTURE DEFINITIONS
 */

/*-----------------------------------------------------------------------------
 *
 * Structure    H5FD__onion_index_entry
 *
 * Purpose:     Map a page in the logical file to a 'physical address' in the
 *              backing store.
 *
 * logi_page:   Page 'id' in the logical file.
 *
 * phys_addr:   Address/offset of start of page in the backing store.
 *
 * Programmer:  Jacob "Jake" Smith
 *              July 2020
 *
 *-----------------------------------------------------------------------------
 */
struct H5FD__onion_index_entry {
    uint64_t    logi_page;
    uint64_t    phys_addr;
};

/*-----------------------------------------------------------------------------
 *
 * Structure    H5FD__onion_archival_index
 *
 * Purpose:     Encapsulate archival index and associated data.
 *              Convenience structure with sanity-checking components.
 *
 * magic:       "Magic number" identifying struct.
 *              Must equal H5FD__ONION_ARCHIVAL_INDEX_MAGIC to be considered
 *              valid.
 *
 * version:     Future-proofing identifier. Informs struct membership.
 *              Must equal H5FD__ONION_ARCHIVAL_INDEX_VERSION_CURR to be
 *              considered valid.
 *
 * page_size:   Interval to which the `logi_page` component of each list
 *              entry must align.
 *              Value is taken from the onion history data; must not change
 *              following onionization or file or creation of onion file.
 *
 * n_entries:   Number of entries in the list.
 *
 * list:        Pointer to array of archival index entries.
 *              Cannot be NULL.
 *              Entries must be sorted by `logi_page_id` in ascending order.
 *
 * Programmer:  Jacob "Jake" Smith
 *              July 2020
 *
 *-----------------------------------------------------------------------------
 */
struct H5FD__onion_archival_index {
    uint32_t    magic;
    uint8_t     version;
    uint32_t    page_size_log2;
    uint64_t    n_entries;
    struct H5FD__onion_index_entry *list;
};

/* data structure for storing index entries at a hash key collision */
/* version 1 implements a singly-linked list */
struct H5FD__onion_revision_index_hash_chain_node {
    uint32_t    magic;
    uint8_t     version;
    struct H5FD__onion_index_entry      entry_data;
    struct H5FD__onion_revision_index_hash_chain_node  *next;
};

typedef struct H5FD__onion_revision_index {
    uint32_t    magic;
    uint8_t     version;
    uint32_t    page_size_log2;
    uint64_t    n_entries; /* count of all entries in table */
    uint64_t    _hash_table_size; /* 'slots' in hash table */
    uint64_t    _hash_table_size_log2; /* 2^(n) -> 'slots' in hash table */
    uint64_t    _hash_table_n_keys_populated; /* count of slots not NULL */
    struct H5FD__onion_revision_index_hash_chain_node **_hash_table;
} H5FD__onion_revision_index_t;

/* In-memory representation of the on-store onion history file header.
 */
struct H5FD__onion_history_header {
    uint32_t    magic;
    uint8_t     version;
    uint32_t    flags; /* at most three bytes used! */
    uint32_t    page_size;
    uint64_t    origin_eof; /* size of the 'original' canonical file */
    uint64_t    whole_history_addr;
    uint64_t    whole_history_size;
    uint32_t    checksum;
};

/* In-memory representation of the on-store revision record.
 */
struct H5FD__onion_revision_record {
    uint32_t    magic;
    uint8_t     version;
    uint64_t    revision_id;
    uint64_t    parent_revision_id;
    char        time_of_creation[16];
    uint64_t    logi_eof;
    uint32_t    user_id;
    uint32_t    username_size;
    uint32_t    comment_size;
    struct H5FD__onion_archival_index   archival_index;
    char       *username;
    char       *comment;
    uint32_t    checksum;
};

/* In-memory representation of the on-store revision record pointer.
 * Used in the whole-history.
 */
struct H5FD__onion_record_pointer {
    uint64_t    phys_addr;
    uint64_t    record_size;
    uint32_t    checksum;
};

/* In-memory representation of the on-store whole-history record/summary.
 */
struct H5FD__onion_whole_history {
    uint32_t    magic;
    uint8_t     version;
    uint64_t    n_revisions;
    struct H5FD__onion_record_pointer  *record_pointer_list;
    uint32_t    checksum;
};

#ifdef __cplusplus
extern "C" {
#endif

/*
 * INTERNAL FUNCTION DECLARATIONS
 */

H5_DLL int H5FD_onion_archival_index_is_valid(
        const struct H5FD__onion_archival_index *);
H5_DLL int H5FD_onion_archival_index_find(
        const struct H5FD__onion_archival_index *, uint64_t,
        const struct H5FD__onion_index_entry **);

H5_DLL struct H5FD__onion_revision_index * H5FD_onion_revision_index_init(
        uint32_t page_size);
H5_DLL herr_t H5FD_onion_revision_index_destroy(
        H5FD__onion_revision_index_t *);
H5_DLL herr_t H5FD_onion_revision_index_insert(
        H5FD__onion_revision_index_t *,
        const struct H5FD__onion_index_entry *);
H5_DLL int H5FD_onion_revision_index_find(
        const H5FD__onion_revision_index_t *, uint64_t,
        const struct H5FD__onion_index_entry **);

H5_DLL herr_t H5FD_onion_merge_revision_index_into_archival_index(
        const H5FD__onion_revision_index_t *,
        struct H5FD__onion_archival_index *);

H5_DLL uint64_t H5FD_onion_history_header_decode(unsigned char *,
        struct H5FD__onion_history_header *);
H5_DLL uint64_t H5FD_onion_history_header_encode(
        struct H5FD__onion_history_header *, unsigned char *, uint32_t *);

H5_DLL uint64_t H5FD_onion_revision_record_decode(unsigned char *,
        struct H5FD__onion_revision_record *);
H5_DLL uint64_t H5FD_onion_revision_record_encode(
        struct H5FD__onion_revision_record *, unsigned char *, uint32_t *);

H5_DLL uint64_t H5FD_onion_whole_history_decode(unsigned char *,
        struct H5FD__onion_whole_history *);
H5_DLL uint64_t H5FD_onion_whole_history_encode(
        struct H5FD__onion_whole_history *, unsigned char *, uint32_t *);

#ifdef __cplusplus
}
#endif


#endif /* H5_HAVE_ONION_VFD defined */

#endif /* H5FDonion_priv_H not defined */

