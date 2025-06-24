#!/bin/bash
> blocks_info.txt

numOfBlocks="${1:-3}"
#get argument from the suer and save it, if no argument is given default will be 3 latest blocks

latest_block_url=$(wget -qO- https://api.blockcypher.com/v1/btc/main | sed 's/[}", {\t]//g' | grep -E "latest_url" | cut -d':' -f2-)
latest_block_info=$(wget -qO- "$latest_block_url")

for ((i=0; i<"$numOfBlocks"; i++)); do
echo "$latest_block_info" | sed 's/[" \t,}{]//g' | grep -E "^hash" >> blocks_info.txt
echo "$latest_block_info" | sed 's/[" \t,}{]//g' | grep -E "^height" >> blocks_info.txt
echo "$latest_block_info" | sed 's/[" \t,}{]//g' | grep -E "^total" >> blocks_info.txt
echo "$latest_block_info" | sed 's/[" \t,}{]//g' | grep -E "^time" >> blocks_info.txt
echo "$latest_block_info" | sed 's/[" \t,}{]//g' | grep -E "^relayed_by" >> blocks_info.txt
echo "$latest_block_info" | sed 's/[" \t,}{]//g' | grep -E "^prev_block\b" >> blocks_info.txt

latest_block_url=$(echo "$latest_block_info" | sed 's/[}", {\t]//g' | grep -E "prev_block_url" | cut -d':' -f2-)
latest_block_info=$(wget -qO- "$latest_block_url")

if [ "$i" -ne "$((numOfBlocks - 1))" ]; then
echo -e "\t\t|\n\t\t|\n\t\t|\n\t\tV" >> blocks_info.txt
fi
done
