#include "io61.h"

// Usage: ./gather61 [-b BLOCKSIZE] [-o OUTFILE] [FILE1 FILE2...]
//    Copies the input FILEs to OUTFILE, alternating between
//    FILEs with every block. (I.e., read a block from FILE1, then
//    a block from FILE2, etc.) This is a "gather" I/O pattern: many
//    input files are gathered into a single output file.
//    Default BLOCKSIZE is 1.

int main(int argc, char* argv[]) {
    // Parse arguments
    io61_arguments args = io61_parse_arguments(argc, argv, "b:o:#");
    size_t block_size = args.block_size ? args.block_size : 1;

    // Allocate buffer, open files
    char* buf = (char*) malloc(block_size);
    int nfiles = args.n_input_files;

    io61_profile_begin();
    io61_file** infs = (io61_file**) calloc(nfiles, sizeof(io61_file*));
    for (int i = 0; i < nfiles; ++i)
        infs[i] = io61_open_check(args.input_files[i], O_RDONLY);
    io61_file* outf = io61_open_check(args.output_file,
                                      O_WRONLY | O_CREAT | O_TRUNC);

    // Copy file data
    int whichf = 0, ndeadfiles = 0;
    while (ndeadfiles != nfiles) {
        if (infs[whichf]) {
            ssize_t amount = io61_read(infs[whichf], buf, block_size);
            if (amount <= 0) {
                io61_close(infs[whichf]);
                infs[whichf] = NULL;
                ++ndeadfiles;
            } else
                io61_write(outf, buf, amount);
        }
        whichf = (whichf + 1) % nfiles;
    }

    io61_close(outf);
    io61_profile_end();
    free(infs);
    free(buf);
}
