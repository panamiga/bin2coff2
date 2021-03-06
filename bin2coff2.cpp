/*
 * bin2coff2: converts a data object into a Win32 linkable COFF binary object
 * like ld -r -o binary outfile.o infile does.
 * Copyright (c) 2017-2018 panamiga <el_@inbox.ru>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* 
 * References:
 * http://pete.akeo.ie/2011/11/bin2coff.html (base of this)
 * http://www.vortex.masmcode.com/ (another bin2coff, without source)
 * http://msdn.microsoft.com/en-us/library/ms680198.aspx
 * http://webster.cs.ucr.edu/Page_TechDocs/pe.txt
 * http://www.delorie.com/djgpp/doc/coff/
 * http://pierrelib.pagesperso-orange.fr/exec_formats/MS_Symbol_Type_v1.0.pdf
 */


#include <string.h>
#include <stdint.h>

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <cctype>

#define SIZE_TYPE                         uint32_t

#define IMAGE_SIZEOF_SHORT_NAME             8

/* File header defines */
#define IMAGE_FILE_MACHINE_ANY             0x0000
#define IMAGE_FILE_MACHINE_I386             0x014c
#define IMAGE_FILE_MACHINE_IA64             0x0200
#define IMAGE_FILE_MACHINE_AMD64         0x8664

#define IMAGE_FILE_RELOCS_STRIPPED         0x0001
#define IMAGE_FILE_EXECUTABLE_IMAGE         0x0002
#define IMAGE_FILE_LINE_NUMS_STRIPPED     0x0004
#define IMAGE_FILE_LOCAL_SYMS_STRIPPED     0x0008
#define IMAGE_FILE_AGGRESIVE_WS_TRIM     0x0010        /* Obsolete */
#define IMAGE_FILE_LARGE_ADDRESS_AWARE     0x0020
#define IMAGE_FILE_16BIT_MACHINE         0x0040
#define IMAGE_FILE_BYTES_REVERSED_LO     0x0080        /* Obsolete */
#define IMAGE_FILE_32BIT_MACHINE         0x0100
#define IMAGE_FILE_DEBUG_STRIPPED         0x0200
#define IMAGE_FILE_REM_RUN_FROM_SWAP     0x0400
#define IMAGE_FILE_NET_RUN_FROM_SWAP     0x0800
#define IMAGE_FILE_SYSTEM                 0x1000
#define IMAGE_FILE_DLL                     0x2000
#define IMAGE_FILE_UP_SYSTEM_ONLY         0x4000
#define IMAGE_FILE_BYTES_REVERSED_HI     0x8000        /* Obsolete */

/* Section header defines */
#define IMAGE_SCN_TYPE_REG                 0x00000000    /* Reserved */
#define IMAGE_SCN_TYPE_DSECT             0x00000001    /* Reserved */
#define IMAGE_SCN_TYPE_NOLOAD             0x00000002    /* Reserved */
#define IMAGE_SCN_TYPE_GROUP             0x00000003    /* Reserved */
#define IMAGE_SCN_TYPE_NO_PAD             0x00000008    /* Obsolete */
#define IMAGE_SCN_TYPE_COPY                 0x00000010    /* Reserved */
#define IMAGE_SCN_CNT_CODE                 0x00000020
#define IMAGE_SCN_CNT_INITIALIZED_DATA     0x00000040
#define IMAGE_SCN_CNT_UNINITIALIZED_DATA 0x00000080
#define IMAGE_SCN_LNK_OTHER                 0x00000100    /* Reserved */
#define IMAGE_SCN_LNK_INFO                 0x00000200
#define IMAGE_SCN_TYPE_OVER                 0x00000400    /* Reserved */
#define IMAGE_SCN_LNK_REMOVE             0x00000800
#define IMAGE_SCN_LNK_COMDAT             0x00001000
#define IMAGE_SCN_MEM_FARDATA             0x00008000    /* Reserved */
#define IMAGE_SCN_MEM_PURGEABLE             0x00020000    /* Reserved */
#define IMAGE_SCN_MEM_16BIT                 0x00020000    /* Reserved */
#define IMAGE_SCN_MEM_LOCKED             0x00040000    /* Reserved */
#define IMAGE_SCN_MEM_PRELOAD             0x00080000    /* Reserved */
#define IMAGE_SCN_ALIGN_1BYTES             0x00100000
#define IMAGE_SCN_ALIGN_2BYTES             0x00200000
#define IMAGE_SCN_ALIGN_4BYTES             0x00300000
#define IMAGE_SCN_ALIGN_8BYTES             0x00400000
#define IMAGE_SCN_ALIGN_16BYTES             0x00500000
#define IMAGE_SCN_ALIGN_32BYTES             0x00600000
#define IMAGE_SCN_ALIGN_64BYTES             0x00700000
#define IMAGE_SCN_ALIGN_128BYTES         0x00800000
#define IMAGE_SCN_ALIGN_256BYTES         0x00900000
#define IMAGE_SCN_ALIGN_512BYTES         0x00A00000
#define IMAGE_SCN_ALIGN_1024BYTES         0x00B00000
#define IMAGE_SCN_ALIGN_2048BYTES         0x00C00000
#define IMAGE_SCN_ALIGN_4096BYTES         0x00D00000
#define IMAGE_SCN_ALIGN_8192BYTES         0x00E00000
#define IMAGE_SCN_ALIGN_MASK             0x00F00000
#define IMAGE_SCN_LNK_NRELOC_OVFL         0x01000000
#define IMAGE_SCN_MEM_DISCARDABLE         0x02000000
#define IMAGE_SCN_MEM_NOT_CACHED         0x04000000
#define IMAGE_SCN_MEM_NOT_PAGED             0x08000000
#define IMAGE_SCN_MEM_SHARED             0x10000000
#define IMAGE_SCN_MEM_EXECUTE             0x20000000
#define IMAGE_SCN_MEM_READ                 0x40000000
#define IMAGE_SCN_MEM_WRITE                 0x80000000

/* Symbol entry defines */
#define IMAGE_SYM_UNDEFINED                 ((int16_t)0)
#define IMAGE_SYM_ABSOLUTE                 ((int16_t)-1)
#define IMAGE_SYM_DEBUG                     ((int16_t)-2)

#define IMAGE_SYM_TYPE_NULL                 0x0000
#define IMAGE_SYM_TYPE_VOID                 0x0001
#define IMAGE_SYM_TYPE_CHAR                 0x0002
#define IMAGE_SYM_TYPE_SHORT             0x0003
#define IMAGE_SYM_TYPE_INT                 0x0004
#define IMAGE_SYM_TYPE_LONG                 0x0005
#define IMAGE_SYM_TYPE_FLOAT             0x0006
#define IMAGE_SYM_TYPE_DOUBLE             0x0007
#define IMAGE_SYM_TYPE_STRUCT             0x0008
#define IMAGE_SYM_TYPE_UNION             0x0009
#define IMAGE_SYM_TYPE_ENUM                 0x000A
#define IMAGE_SYM_TYPE_MOE                 0x000B
#define IMAGE_SYM_TYPE_BYTE                 0x000C
#define IMAGE_SYM_TYPE_WORD                 0x000D
#define IMAGE_SYM_TYPE_UINT                 0x000E
#define IMAGE_SYM_TYPE_DWORD             0x000F
#define IMAGE_SYM_TYPE_PCODE             0x8000

#define IMAGE_SYM_DTYPE_NULL             0
#define IMAGE_SYM_DTYPE_POINTER             1
#define IMAGE_SYM_DTYPE_FUNCTION         2
#define IMAGE_SYM_DTYPE_ARRAY             3

#define IMAGE_SYM_CLASS_END_OF_FUNCTION     0xFF
#define IMAGE_SYM_CLASS_NULL             0x00
#define IMAGE_SYM_CLASS_AUTOMATIC         0x01
#define IMAGE_SYM_CLASS_EXTERNAL         0x02
#define IMAGE_SYM_CLASS_STATIC             0x03
#define IMAGE_SYM_CLASS_REGISTER         0x04
#define IMAGE_SYM_CLASS_EXTERNAL_DEF     0x05
#define IMAGE_SYM_CLASS_LABEL             0x06
#define IMAGE_SYM_CLASS_UNDEFINED_LABEL     0x07
#define IMAGE_SYM_CLASS_MEMBER_OF_STRUCT 0x08
#define IMAGE_SYM_CLASS_ARGUMENT         0x09
#define IMAGE_SYM_CLASS_STRUCT_TAG         0x0A
#define IMAGE_SYM_CLASS_MEMBER_OF_UNION     0x0B
#define IMAGE_SYM_CLASS_UNION_TAG         0x0C
#define IMAGE_SYM_CLASS_TYPE_DEFINITION     0x0D
#define IMAGE_SYM_CLASS_UNDEFINED_STATIC 0x0E
#define IMAGE_SYM_CLASS_ENUM_TAG         0x0F
#define IMAGE_SYM_CLASS_MEMBER_OF_ENUM     0x10
#define IMAGE_SYM_CLASS_REGISTER_PARAM     0x11
#define IMAGE_SYM_CLASS_BIT_FIELD         0x12
#define IMAGE_SYM_CLASS_FAR_EXTERNAL     0x44
#define IMAGE_SYM_CLASS_BLOCK             0x64
#define IMAGE_SYM_CLASS_FUNCTION         0x65
#define IMAGE_SYM_CLASS_END_OF_STRUCT     0x66
#define IMAGE_SYM_CLASS_FILE             0x67
#define IMAGE_SYM_CLASS_SECTION             0x68
#define IMAGE_SYM_CLASS_WEAK_EXTERNAL     0x69
#define IMAGE_SYM_CLASS_CLR_TOKEN         0x6B

#pragma pack(push, 2)

/* Microsoft COFF File Header */
typedef struct {
    uint16_t    Machine;
    uint16_t    NumberOfSections;
    uint32_t    TimeDateStamp;
    uint32_t    PointerToSymbolTable;
    uint32_t    NumberOfSymbols;
    uint16_t    SizeOfOptionalHeader;
    uint16_t    Characteristics;
} IMAGE_FILE_HEADER;

/* Microsoft COFF Section Header */
typedef struct {
    char Name[IMAGE_SIZEOF_SHORT_NAME];
    uint32_t    VirtualSize;
    uint32_t    VirtualAddress;
    uint32_t    SizeOfRawData;
    uint32_t    PointerToRawData;
    uint32_t    PointerToRelocations;
    uint32_t    PointerToLinenumbers;
    uint16_t    NumberOfRelocations;
    uint16_t    NumberOfLinenumbers;
    uint32_t    Characteristics;
} IMAGE_SECTION_HEADER;

/* Microsoft COFF Symbol Entry */
typedef struct {
    union {
        char    ShortName[IMAGE_SIZEOF_SHORT_NAME];
        struct {
            uint32_t Zeroes;
            uint32_t Offset;
        } LongName;
    } N;
    int32_t        Value;
    int16_t        SectionNumber;
    uint16_t    Type;
    uint8_t        StorageClass;
    uint8_t        NumberOfAuxSymbols;
} IMAGE_SYMBOL;

/* COFF String Table */
typedef struct {
    uint32_t    TotalSize;
    char        Strings[0];
} IMAGE_STRINGS;

#pragma pack(pop)

int main (int argc, char *argv[])
{
    IMAGE_FILE_HEADER* file_header;
    IMAGE_SECTION_HEADER* section_header;
    IMAGE_SYMBOL* symbol_table;
    IMAGE_STRINGS* string_table;
    SIZE_TYPE* data_size;

    if (argc != 3) {
        std::cerr << "\nUsage: bin2coff2 outfile.o infile\n\n";
        std::cerr << "  outfile.o  : target object file, in MS COFF format\n";
        std::cerr << "  infile     : source binary data\n";        
        std::cerr << "This tool do the same as ld -r -b binary -o outfile.o infile do but for MSVC\n";
        std::cerr << "Access from C source is:\n\n";
        std::cerr << "    extern char _binary_*MANGLED_PATH*_start   /* the first char of binary data */\n";
        std::cerr << "    extern char _binary_*MANGLED_PATH*_end     /* the last char of binary data */\n\n";
        return 1;
    }

    const uint16_t endian_test = 0xBE00;
    if (((uint8_t*)&endian_test)[0] == 0xBE) 
    {
        std::cerr << "\nThis program is not compatible with Big Endian architectures." << std::endl;
        std::cerr << "You are welcome to modify the sourcecode (GPLv3+) to make it so." << std::endl;
        return 1;
    }
    
    std::ofstream dest(argv[1], std::ios::binary);
    if (!dest) 
    {
         std::cerr << "Couldn't open file '" << argv[1] << "' for writing." << std::endl;
         return 1;
    }
    
    std::ifstream source(argv[2], std::ios::binary | std::ios::ate);        
    if (!source) 
    {
         std::cerr << "Couldn't open file '" << argv[2] << "'." << std::endl;
         return 1;
    }
    
    size_t size = source.tellg();
    source.seekg(0);

    /* Mangle source filename */
    std::string label(argv[2]);
    for(char &c: label)
    {
        c = std::isalnum(c) ? c : '_';
    }
    
    std::string start_label = std::string("?_binary_") + label + "_start@@3DB";
    std::string stop_label = std::string("?_binary_") + label + "_end@@3DB";
    
    /* Allocate buffer and set data pointers */
    size_t alloc_size = sizeof(IMAGE_FILE_HEADER) + sizeof(IMAGE_SECTION_HEADER) + sizeof(SIZE_TYPE) + 2*sizeof(IMAGE_SYMBOL) + sizeof(IMAGE_STRINGS);    
    alloc_size += start_label.length() + 1;
    alloc_size += stop_label.length() + 1;    
    
    std::vector<uint8_t> buf(alloc_size, 0);
    uint8_t* buffer = buf.data();      
    
    file_header = (IMAGE_FILE_HEADER*)buffer;    
    buffer += sizeof(IMAGE_FILE_HEADER);
    
    section_header = (IMAGE_SECTION_HEADER*)buffer;
    buffer += sizeof(IMAGE_SECTION_HEADER);
    
    data_size = (SIZE_TYPE*)buffer;
    buffer += sizeof(SIZE_TYPE);
    
    symbol_table = (IMAGE_SYMBOL*)buffer;
    buffer += 2*sizeof(IMAGE_SYMBOL);
    
    string_table = (IMAGE_STRINGS*)buffer;

    /* Populate file header */
    file_header->Machine = IMAGE_FILE_MACHINE_ANY;
    file_header->NumberOfSections = 1;
    file_header->PointerToSymbolTable = sizeof(IMAGE_FILE_HEADER) + sizeof(IMAGE_SECTION_HEADER) + (uint32_t)size + 4;
    file_header->NumberOfSymbols = 2;
    file_header->Characteristics = IMAGE_FILE_LINE_NUMS_STRIPPED;

    /* Populate data section header */
    strncpy(section_header->Name, ".data", IMAGE_SIZEOF_SHORT_NAME);
    section_header->SizeOfRawData = (uint32_t)size + 4;
    section_header->PointerToRawData = sizeof(IMAGE_FILE_HEADER) + sizeof(IMAGE_SECTION_HEADER);
    section_header->Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_ALIGN_16BYTES | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;

    /* Set data size */
    *data_size = (SIZE_TYPE)size;

    /* Populate symbol table */
    symbol_table[0].N.LongName.Zeroes = 0;
    symbol_table[0].N.LongName.Offset = sizeof(IMAGE_STRINGS);
    
    /* Ideally, we would use (IMAGE_SYM_DTYPE_ARRAY << 8) | IMAGE_SYM_TYPE_BYTE
     * to indicate an array of bytes, but the type is ignored in MS objects. */
    symbol_table[0].Type = IMAGE_SYM_TYPE_NULL;
    symbol_table[0].StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
    symbol_table[0].SectionNumber = 1;
    symbol_table[0].Value = 0;                /* Offset within the section */

    symbol_table[1].N.LongName.Zeroes = 0;
    symbol_table[1].N.LongName.Offset = sizeof(IMAGE_STRINGS) + (uint32_t)start_label.length() + 1;
    
    symbol_table[1].Type = IMAGE_SYM_TYPE_NULL;
    symbol_table[1].StorageClass = IMAGE_SYM_CLASS_EXTERNAL;
    symbol_table[1].SectionNumber = 1;
    symbol_table[1].Value = (int32_t)size;    /* Offset within the section */

    /* Populate string table */
    string_table->TotalSize = sizeof(IMAGE_STRINGS);
            
    strcpy(string_table->Strings, start_label.c_str());
    string_table->TotalSize += (uint32_t)start_label.length() + 1;    
    
    strcpy(&string_table->Strings[string_table->TotalSize - sizeof(IMAGE_STRINGS)], stop_label.c_str());
    string_table->TotalSize += (uint32_t)stop_label.length() + 1;
    
    /* Finally write the data */
    dest.write((char*)file_header, sizeof(IMAGE_FILE_HEADER));
    dest.write((char*)section_header, sizeof(IMAGE_SECTION_HEADER));
    
    dest << source.rdbuf();
    
    dest.write((char*)data_size, sizeof(SIZE_TYPE));
    dest.write((char*)symbol_table, 2*sizeof(IMAGE_SYMBOL));
    dest.write((char*)string_table, string_table->TotalSize);
    
    source.close();
    dest.close();
    
    if(source && dest)
    {    
        std::cout << "Successfully created COFF object file '" << argv[1] << "'." << std::endl;
        std::cout << "extern char " << start_label.substr(1, start_label.length() - 6) << "   /* the first char of binary data */" << std::endl;
        std::cout << "extern char " << stop_label.substr(1, stop_label.length() - 6) << "   /* the last char of binary data */" << std::endl;

        return 0;
    } 
    else 
    {
        std::cerr << "Failed to create COFF object file '" << argv[1] << "'." << std::endl;
        return 1;
    }
        
}

