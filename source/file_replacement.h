#include <switch_min.h>
// #include <cstdlib>

#include "useful/useful.h"
#include "useful/crc32.h"

#include "saltysd/saltysd_core.h"
#include "saltysd/saltysd_ipc.h"
#include "saltysd/saltysd_dynamic.h"

// FileHandle == nn::fs::detail::FileSystemAccessor*

typedef struct FileSystemAccessor
{
    void* unk1; // nn::fs::detail::FileAccessor
    void* unk2;
    void* unk3;
    void* mutex; // nn::os::Mutex
    uint64_t val1;
    uint64_t mode;
} FileSystemAccessor;

namespace nn::fs
{
    struct FileHandle
    {
        FileSystemAccessor* wrap;
        
        bool operator==(const FileHandle& comp)
        {
            return (wrap == comp.wrap);
        }
    };

    extern Result OpenFile(nn::fs::FileHandle* out, char const* path, int mode) asm("_ZN2nn2fs8OpenFileEPNS0_10FileHandleEPKci") LINKABLE;
    extern Result ReadFile(unsigned long* read, nn::fs::FileHandle handle, long offset, void* out, unsigned long size) asm("_ZN2nn2fs8ReadFileEPmNS0_10FileHandleElPvm") LINKABLE;
    extern Result ReadFile(nn::fs::FileHandle handle, long offset, void* out, unsigned long size) asm("_ZN2nn2fs8ReadFileENS0_10FileHandleElPvm") LINKABLE;
}

typedef struct arc_header
{
    uint64_t magic;
    uint64_t offset_1;
    uint64_t offset_2;
    uint64_t offset_3;
    uint64_t offset_4;
    uint64_t offset_5;
    uint64_t offset_6;
} arc_header;

typedef struct offset5_header
{
    uint64_t total_size;
    uint32_t folder_entries;
    uint32_t file_entries;
    uint32_t hash_entries;
} offset5_header;

typedef struct offset4_header
{
    uint32_t total_size;
    uint32_t entries_big;
    uint32_t entries_bigfiles_1;
    uint32_t tree_entries;
    
    uint32_t suboffset_entries;
    uint32_t file_lookup_entries;
    uint32_t folder_hash_entries;
    uint32_t tree_entries_2;
    
    uint32_t entries_bigfiles_2;
    uint32_t post_suboffset_entries;
    uint32_t alloc_alignment;
    uint32_t unk10;
    
    uint8_t weird_hash_entries;
    uint8_t unk11;
    uint8_t unk12;
    uint8_t unk13;
} offset4_header;

typedef struct offset4_ext_header
{
    uint32_t bgm_unk_movie_entries;
    uint32_t entries;
    uint32_t entries_2;
    uint32_t num_files;
} offset4_ext_header;

typedef struct entry_triplet
{
    uint64_t hash : 40;
    uint64_t meta : 24;
    uint32_t meta2;
} __attribute__((packed)) entry_triplet;

typedef struct entry_pair
{
    uint64_t hash : 40;
    uint64_t meta : 24;
} __attribute__((packed)) entry_pair;

typedef struct file_pair
{
    uint64_t size;
    uint64_t offset;
} file_pair;

typedef struct big_hash_entry
{
    entry_pair path;
    entry_pair folder;
    entry_pair parent;
    entry_pair hash4;
    uint32_t suboffset_start;
    uint32_t num_files;
    uint32_t unk3;
    uint16_t unk4;
    uint16_t unk5;
    uint8_t unk6;
    uint8_t unk7;
    uint8_t unk8;
    uint8_t unk9;
} big_hash_entry;

typedef struct big_file_entry
{
    uint64_t offset;
    uint32_t decomp_size;
    uint32_t comp_size;
    uint32_t suboffset_index;
    uint32_t files;
    uint32_t unk3;
} __attribute__((packed)) big_file_entry;

typedef struct file_entry
{
    uint32_t offset;
    uint32_t comp_size;
    uint32_t decomp_size;
    uint32_t flags;
} file_entry;

typedef struct tree_entry
{
    entry_pair path;
    entry_pair ext;
    entry_pair folder;
    entry_pair file;
    uint32_t suboffset_index;
    uint32_t flags;
} tree_entry;

typedef struct folder_tree_entry
{
    entry_pair path;
    entry_pair parent;
    entry_pair folder;
    uint32_t idx1;
    uint32_t idx2;
} folder_tree_entry;

typedef struct mini_tree_entry
{
    entry_pair path;
    entry_pair folder;
    entry_pair file;
    entry_pair ext;
} mini_tree_entry;

typedef struct hash_bucket
{
    uint32_t index;
    uint32_t num_entries;
} hash_bucket;

typedef struct offset4_structs
{
    void* off4_data;
    offset4_header* header;
    offset4_ext_header* ext_header;
    entry_triplet* bulkfile_category_info;
    entry_pair* bulkfile_hash_lookup;
    entry_triplet* bulk_files_by_name;
    uint32_t* bulkfile_lookup_to_fileidx;
    file_pair* file_pairs;
    entry_triplet* weird_hashes;
    big_hash_entry* big_hashes;
    big_file_entry* big_files;
    entry_pair* folder_hash_lookup;
    tree_entry* tree_entries;
    file_entry* suboffset_entries;
    file_entry* post_suboffset_entries;
    entry_pair* folder_to_big_hash;
    hash_bucket* file_lookup_buckets;
    entry_pair* file_lookup;
    entry_pair* numbers3;
} offset4_structs;

typedef struct offset5_structs
{
    void* off5_data;
    offset5_header* header;
    entry_pair* folderhash_to_foldertree;
    folder_tree_entry* folder_tree;
    entry_pair* entries_13;
    uint32_t* numbers;
    mini_tree_entry* tree_entries;
} offset5_structs;

typedef struct arc_section
{
    uint32_t data_start;
    uint32_t decomp_size;
    uint32_t comp_size;
    uint32_t zstd_comp_size;
} arc_section;

#define TREE_ALIGN_MASK           0x0fffe0
#define TREE_ALIGN_LSHIFT         (5)
#define TREE_SUBOFFSET_MASK       0x000003
#define TREE_SUBOFFSET_IDX        0x000000
#define TREE_SUBOFFSET_EXT_ADD1   0x000001
#define TREE_SUBOFFSET_EXT_ADD2   0x000002
#define TREE_REDIR                0x200000
#define TREE_UNK                  0x100000

#define SUBOFFSET_TREE_IDX_MASK     0x00FFFFFF
#define SUBOFFSET_REDIR             0x40000000
#define SUBOFFSET_UNK_BIT29         0x20000000
#define SUBOFFSET_UNK_BIT27         0x08000000
#define SUBOFFSET_UNK_BIT26         0x04000000

#define SUBOFFSET_COMPRESSION       0x07000000
#define SUBOFFSET_DECOMPRESSED      0x00000000
#define SUBOFFSET_UND               0x01000000
#define SUBOFFSET_COMPRESSED_LZ4    0x02000000
#define SUBOFFSET_COMPRESSED_ZSTD   0x03000000

nn::fs::FileHandle* data_arc_handles = NULL;
u32 num_data_arc_handles = 0;
arc_header arc_head;
offset4_structs off4_structs;
offset5_structs off5_structs;
offset4_header off4_header_orig;

// extern "C" {

// extern void* bsearch (const void* key, const void* base,
//                size_t num, size_t size,
//                int (*compar)(const void*,const void*)) asm("bsearch") LINKABLE;

// }


// void* bsearch_intercept(const void* key, const void* base, size_t num, size_t size, int (*compar)(const void*,const void*))
// {
//     void* ret = bsearch(key, base, num, size, compar);
//     if (*(u32*)key == 0xe03bdf27 || *(u32*)key == 0xccbff852 || *(u32*)key == 0x5cce92a7 || *(u32*)key == 0x5cce92a7 || *(u32*)key == 0x9b741175)
//         debug_log("bsearch(%p (%llx), %p, %u, 0x%zx, %p) -> %p\n", key, *(u64*)key & 0xFFFFFFFFFF, base, num, size, compar, ret);
//     return ret;
// }

Result OpenFile_intercept(nn::fs::FileHandle* out, char const* path, int mode)
{
    Result ret = nn::fs::OpenFile(out, path, mode);
    debug_log("SaltySD Plugin: OpenFile(%llx, \"%s\", %llx) -> %llx,%p\n", out, path, mode, ret, *out);
    
    //TODO: closing
    if (!strcmp(path, "rom:/data.arc"))
    {
        // data_arc_handles = (nn::fs::FileHandle*)realloc(data_arc_handles, ++num_data_arc_handles * sizeof(FileSystemAccessor*));
        // data_arc_handles[num_data_arc_handles-1] = *out;
    }

    return ret;
}

Result ReadFile_intercept(unsigned long* read, nn::fs::FileHandle handle, long offset, void* out, unsigned long size)
{
    Result ret = nn::fs::ReadFile(read, handle, offset, out, size);
    debug_log("SaltySD Plugin: ReadFile(%llx, %llx, %llx, %llx, %llx) -> %llx\n", read, handle, offset, out, size, ret);
    return ret;
}

Result ReadFile_intercept2(nn::fs::FileHandle handle, long offset, void* out, unsigned long size)
{
    Result ret = nn::fs::ReadFile(handle, offset, out, size);
    debug_log("SaltySD Plugin: ReadFile2(%p, %llx, %p, %llx) -> %llx\n", handle, offset, out, size, ret);

    u32 index = 0;
    for (index = 0; index < num_data_arc_handles; index++)
    {
        if (handle == data_arc_handles[index]) break;
    }
    
    // Handle not data.arc
    if (index == num_data_arc_handles) return ret;
    
    if (offset == arc_head.offset_4 + sizeof(offset4_header))
    {
        size -= 0x200000; // alloc adjust
    }
    
    if (offset == 0xBA0ACDF8)
    {
        debug_log("WOLF\nSaltySD Plugin: ReadFile2(%p, %llx, %p, %llx)\n", handle, offset, out, size);

        /*void** tp = (void**)((u8*)armGetTls() + 0x1F8);
        *tp = malloc(0x1000);
        
        FILE* f = fopen("sdmc:/SaltySD/smash/prebuilt/nro/release/lua2cpp_wolf.nro", "rb");
        if (f)
        {
            fseek(f, offset - 0xb7720d08, SEEK_SET);
            size_t read = fread(out, size, 1, f);
            fclose(f);
            
            debug_log("read %zx bytes\n", read);
        }
        else
        {
            debug_log("Failed to open sdmc:/SaltySD/smash/prebuilt/nro/release/lua2cpp_wolf.nro\n");
        }
        
        free(tp);
        return 0;*/
    }

    return ret;
}

int file_replacement()
{   
    // SaltySDCore_ReplaceImport("_ZN2nn2fs8ReadFileEPmNS0_10FileHandleElPvm", (void*)ReadFile_intercept);
    // SaltySDCore_ReplaceImport("_ZN2nn2fs8ReadFileENS0_10FileHandleElPvm", (void*)ReadFile_intercept2);
    // SaltySDCore_ReplaceImport("_ZN2nn2fs8OpenFileEPNS0_10FileHandleEPKci", (void*)OpenFile_intercept);
    // SaltySDCore_ReplaceImport("bsearch", (void*)bsearch_intercept);
}