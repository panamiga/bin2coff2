bin2coff2: converts binary data file to linkable MS COFF object files

The generated object file can then be used with MS compilers (Visual Studio,
WDK) or MinGW (32 or 64 bit). The object file contains 2 variables,
the first and the last char of binary data.

Current limitations:
- only little endian architectures are supported
- only x86 architectures are supported
- source must be 4 GB or less

These limitations can easily be overcome by modifying the source.
You must respect the GPL v3 (or later) license if you do so.

Usage: bin2coff2 outfile.o infile

    outfile.o  : target object file, in MS COFF format
    infile     : source binary data
    
This tool do the same as ld -r -b binary -o outfile.o infile do but for MSVC

Access from C/C++ source is:

    extern char _binary_*MANGLED_PATH*_start   /* the first char of binary data */
    extern char _binary_*MANGLED_PATH*_end     /* the last char of binary data */
