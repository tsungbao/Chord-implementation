#!/bin/bash
# declare -a test_name=('1' '3-1' '3-2') ;

# for name in "${test_name[@]}"
# do
#     for i in $(seq 0 7)
#     do 
#         ./chord 127.0.0.1 $((5057+${i})) & 
#     done 

#     python3 ../test_scripts/test_part_${name}.py > ../test_result${name}

#     kill -15 `ps | awk '{if ($4=="chord") \
#                             print $1}'`
# done

# sleep 1200


for i in $(seq 0 7)
do 
    ./chord 127.0.0.1 $((5057+${i})) & 
done 

tail -F anything