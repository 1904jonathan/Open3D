#source from /bin/bash
modifyInclude() {
	perl -pe "s{^(#include .)$1/}{\$1$2/}"
}

mvDir() {
	sd=$1
	td=$2
	[ -e $sd ]||{ echo "$sd doesn't exist";return;}
	[ -e $td ]&&{ echo "$td already exist";return;}
	git mv "$sd" "$td" ||{ echo "failed mv";return;}
	echo "$1 $2" >> reorg/movedDirs.lst
}

#moves file/dir to new place and records the change
#if you want git to figure out that the move happened, commit and then run applyIncludeChanges
#this way if the files are unchanged git heuristics will work for detecting file movement
mvInclude() {
	sd=cpp/$1
	td=cpp/$2
	[ -e $sd ]||{ echo "$sd doesn't exist";return;}
	[ -e $td ]&&{ echo "$td already exist";return;}
	git mv "$sd" "$td" ||{ echo "failed mv";return;}
	echo "$1 $2" >> reorg/applyIncludes.lst
}

applySingleIncludeChange() {
	find cpp examples -name '*.[ch]*'|while read f
	do
		cat $f |
		modifyInclude "$1" "$2" |
		cat > $f.reorgincl
		diff --brief $f $f.reorgincl > /dev/null || echo $f
		#preserve permissions
		cat $f.reorgincl > $f
		rm $f.reorgincl
	done
	echo "$1 $2" >> reorg/movedIncludes.lst
}

applyAllIncludeChanges() {
	cat reorg/applyIncludes.lst|while read l
	do
		applySingleIncludeChange $l
	done
	rm reorg/applyIncludes.lst
}

