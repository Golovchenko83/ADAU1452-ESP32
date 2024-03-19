#!/bin/bash
file_n="EQ_4000"
stroka=0
kol_s=$(awk 'END{ print NR }' "$file_n".txt)
for ((s = 0;  s < kol_s/5; s++)) do
    #echo "$s"
    for i in 1 2 3 4 5; do
        raw_data=$(awk NR==$i+$stroka "$file_n".txt | head -n5 | awk '{print $2}')
        if [[ $i = 1 ]]; then
            raw_data="{${raw_data}"
        fi
        if [[ $i != 5 ]]; then
            raw_data=$(tr '\r' , <<<"$raw_data")
        else
            raw_data=$(tr -d '\r\n' <<<"$raw_data")
        fi
        #raw_data="${raw_data},"
        data="${data}${raw_data}"
    done
    data="${data}},"
    echo "$data"
    stroka=$stroka+5
    echo -e "$data" >> data_"$file_n".txt
    data=""
done
