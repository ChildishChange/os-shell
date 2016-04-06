
for i in `find . -type f` 
	do
		echo $i
		iconv -f GBK -t UTF-8  $i -o $i
	done

