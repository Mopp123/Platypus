#!/bin/bash

# creates combined header file which can be used to include the whole engine
out_file="Platypus.h"

# Get script's dir
root_dir=$(dirname "$0")
cd $root_dir
root_dir=$(pwd)

src_dir=$root_dir/platypus
target_dir=$src_dir

echo "Building combined header file using files from $src_dir into $target_dir"

cd $target_dir

# Check does the "combined header file" already exist -> if it does delete it
if [ -f $out_file ]
then
    echo "Existing combined header found. Deleting..."
    rm $out_file
fi

# Create the combined header
touch $out_file
echo "#pragma once" >> $out_file
echo '' >> $out_file
find $src_dir -type f -print0 | while read -d $'\0' file; do
    if [[ $file != *"platform"* ]] && [[ $file == *.h ]] && [[ $file != *"Platypus.h" ]]
    then
        echo "#include \"$(realpath -s --relative-to="$src_dir" "$file")\"" >> $out_file
    fi
done
