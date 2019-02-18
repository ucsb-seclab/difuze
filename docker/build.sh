#!/bin/bash
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <target_image_folder>"
    exit 1
fi
if [ ! -d "$1" ]; then
    echo "Provided image folder: $1 doesn't exist."
    exit 2
fi

(
cd $1
docker build -t machiry/$1 .
docker push machiry/$1
)

