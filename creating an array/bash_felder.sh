#!/bin/bash
#echo "Количество старших byte:"
#IFS= read -r sbyte
#echo "$sbyte"
file_n="ФелдрВЧвыход"
stroka=1
pov_b=2
kol_s=$(awk 'END{ print NR }' "$file_n".txt)
for ((s = 0; s < kol_s / pov_b; s++)); do

    raw_data=$(awk NR==$stroka "$file_n".txt | head -n5 | awk '{print $2}')
    raw_data="{${raw_data}"
    raw_data=$(tr -d '\r\n' <<<"$raw_data")
    data="${data}${raw_data}"
    data="${data}},"
    echo "$data"
    stroka=$stroka+$pov_b
    echo -e "$data" >>data_"$file_n".txt
    data=""
done
