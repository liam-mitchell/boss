#!/bin/bash

# Generate an initrd image with directory structure and content equivalent
# to the working directory of the script.

# write-binary-int
# params:
#  $1 file to write to
#  $2 32-bit integer to write to file
write-binary-int() {
    python <<EOF
import struct
with open("$1", "ba") as f:
    f.write(struct.pack('i', $2))
EOF
}

# make-dirfile
# params:
#  $1: directory to create init file for
#  $2: initrd temp dir
make-dirfile() {
    dirfile=$1
    if [ "$dirfile" == "." ]
    then
        dirfile=$(basename $PWD)
    fi

    echo -ne > `dirfile-name $1 $2`
}

# dirfile-name
# params:
#  $1: file to retrieve dirfile name for
#  $2: initrd temp dir
dirfile-name() {
    dirfile=$1

    if [ "$dirfile" == "." ]
    then
        dirfile=$(basename $PWD)
    fi

    echo -ne "$2/$(basename $dirfile)"
}

# make-dirfiles
# params:
#  $1: initrd temp dir
#  $2: array of files indexed by ino
make-dirfiles() {
    inode=`add-direntries . $1 $2`

    for d in `find . -type d | grep -v "^\.$" | sed s:^..::g`
    do
        inode=`add-direntries $d $1 $2`
    done
}

# find-inode
# params:
#  $1: filename
#  $2: array of filenames indexed by ino
find-inode() {
    i=1
    inodes=$2
    for f in "${inodes[@]}"
    do
        if [[ "$f" == *"$1" ]]
        then
            echo -n $i
        fi
        i=$((i + 1))
    done
}

# add-direntries
# params:
#  $1: directory to add entries for
#  $2: initrd temp dir
#  $3: array of files indexed by ino
add-direntries() {
    make-dirfile $(basename $1) $2
    dirfile=`dirfile-name $(basename $1) $2`
    
    for f in `find $1 -maxdepth 1 -mindepth 1`
    do
        f=`echo $f | cut -f2 -d'/'`
        add-direntry $dirfile $f `find-inode $f $3`
    done

    echo $inode
}

# add-direntry
# params:
#  $1 dirfile to add to
#  $2 name of entry
#  $3 inode number of entry
add-direntry() {
    dirfile=$1
    entry=$2
    inode=$3
    
    write-binary-int $dirfile $inode

    echo -n $entry >> $dirfile
    
    i=$((${#entry} % 4))
    
    while [ $i -lt 4 ]
    do
        echo -ne "\\0" >> $dirfile
        i=$(($i + 1))
    done
}

# write-super
# params:
#  $1: initrd filename
write-super() {
    count=`find . | wc -l`
    write-binary-int $1 $count
}

# read-super
# params:
#  $1: initrd filename

read-super() {
    python <<EOF
import struct
with open("$1", "rb") as f:
    print(struct.unpack("<I",f.read(4))[0])
EOF
}

# params:
#  $1: initrd .img filename
#  $2: initrd temp directory
#  $3: file or directory to write
#  $4: offset to start at
write-inode() {
    data=$3
    flags=1

    if [ -d $data ]
    then
        data=`dirfile-name $data $2`
        flags=2
    fi

    length=`wc -c $data | cut -f1 -d' '`
    offset=$4

    write-binary-int $1 $offset
    write-binary-int $1 $length
    write-binary-int $1 $flags

    echo $(($offset + $length))
}

# write-inodes
# params:
#  $1: initrd .img filename
#  $2: initrd temp directory
#  $3: list of files to write data for
write-inodes() {
    offset=$((`find . | wc -l` * 12 + 4))

    for f in `find .`
    do
        echo "[gen-initrd] Writing inode data for $f"
        offset=`write-inode $1 $2 $f $offset`
    done
}

# write-data
# params:
#  $1: initrd .img filename
#  $2: file to write
#  $3: initrd temp directory
write-data() {
    file=$2

    if [ -d $file ]
    then
        file=`dirfile-name $file $3`
    fi

    cat $file >> $1
}

# write-datas
# params:
#  $1: initrd .img filename
#  $2: initrd temp directory
#  $3: list of files to write data for
write-datas() {
    for f in `find .`
    do
        echo "[gen-initrd] Writing file data for $f"
        write-data $1 $f $2
    done
}

list-files() {
    find . -mindepth 1
}

initdir=`mktemp -d /tmp/initrd-out.XXX`

while getopts "o:d:" opt;
do
    case $opt in
        o)
            img=$(realpath $OPTARG);;
        d)
            dir=$(realpath $OPTARG);;
        \?)
            echo "invalid option: $OPTARG";;
    esac
done              

echo "[gen-initrd] Writing initrd image to $img"
echo "[gen-initrd] Copying filesystem structure from $dir"

cd $dir

inodes=( $(list-files) )

make-dirfiles $initdir $inodes
>$img
write-super $img
write-inodes $img $initdir $inodes
write-datas $img $initdir $inodes
