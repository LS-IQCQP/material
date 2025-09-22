#!/bin/bash

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 /path/to/instance.mps.gz /path/to/results"
    exit 1
fi

input_file="$1"
output_dir="$2"

if [ ! -f "$input_file" ]; then
    echo "Error: Input file $input_file does not exist"
    exit 1
fi

mkdir -p "$output_dir"
filename=$(basename "$input_file" .mps.gz)

script_dir=$(dirname "$(readlink -f "$0")")
temp_dir=$(mktemp -d -p "$script_dir")
trap 'rm -rf "$temp_dir"' EXIT

gunzip -c "$input_file" > "$temp_dir/${filename}.mps"
"$script_dir/Qseek" "$temp_dir/${filename}.mps" "$output_dir/$filename" 300