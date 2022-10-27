#!/bin/python

#This script compiles shaders in GLSL into SPIR-V
# Arg1 - compiler path
# Arg2 - source directory containing shader files in GLSL
# Arg3 - target directory to put compiled shaders into

import sys
import os
import subprocess
import glob

def inDir():
    indir = sys.argv[2]

    #strip trailing / if present
    if indir[-1] == '/':
        indir = indir[0:-1]

    return indir

def generateIncludeFileList(filepath, file_list):
    if not os.path.isfile(filepath) or not os.access(filepath, os.R_OK):
        sys.exit("Error when generating include file list: \"" + filepath + "\" is not a readable file.")

    with open(filepath) as file:
        for line in file:
            if line.startswith("#include "):
                include_file_name = line.split()[1].strip('"')
                include_file_path = inDir() + "/" + include_file_name
                file_list.append(include_file_path)
                generateIncludeFileList(include_file_path, file_list)

def main():
    arg_count = len(sys.argv) - 1

    if arg_count < 3:
        sys.exit("Error: 3 arguments required. (" + str(arg_count) + " provided)")

    compiler = sys.argv[1]
    indir = inDir()
    outdir = sys.argv[3]

    if not os.path.isdir(indir):
        sys.exit("Error: \"" + indir + "\" is not a directory.")

    if not os.path.isdir(outdir):
        os.mkdir(outdir)

    if not os.path.isfile(compiler) or not os.access(compiler, os.X_OK):
        sys.exit("Error: \"" + compiler + "\" is not an executable file.")

    if outdir[-1] == '/':
        outdir = outdir[0:-1]

    #TODO: find and remove existing .spv files that have no matching source files? (so remove old .spv that is no longer needed)
    #for f in glob.glob(outdir + "/*.spv"):
        #os.remove(f)

    ret = 0

    #for each file in the source directory
    for file in glob.glob(indir + "/*"):
        #get the file name and extension
        name, ext = os.path.splitext(file)
        ext = ext[1:]
        name = os.path.basename(name)

        supported_extensions = ["vert", "tesc", "tese", "geom", "frag", "comp"]

        if ext in supported_extensions:
            outfile = outdir + "/" + name + ".spv"

            source_files_to_check = [file]
            generateIncludeFileList(file, source_files_to_check)

            if os.path.isfile(outfile):
                time_bin = os.path.getmtime(outfile)

                skip = True

                for source_file in source_files_to_check:
                    time_source = os.path.getmtime(source_file)
                    if time_bin < time_source:
                        skip = False
                        break

                if skip:
                    continue
            
            print("Compiling " + file, flush = True)
            success = subprocess.call([compiler, file, "-o", outfile])

            if success != 0:
                ret = -1

        elif ext != "h": #accept .h extension (include headers) but don't compile
            print("WARNING: " + file + " has unsupported extension. Skipping...", flush = True)

    sys.exit(ret)

if __name__ == "__main__":
    main()
