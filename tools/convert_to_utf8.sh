#!/bin/sh
# !!!!!!!!!!! ne pas oublier avant : apt-get install moreutils !!!!!!!!!!!!
# encodage de depart
encodeFrom='ISO-8859-1'
# encodage voulu
encodeTo='UTF-8'
# application du script sur tous les fichiers
for filename in `find . -type f -name '*'`
do
isutf8 $filename
if [ $? = 0 ]
then
echo "fichier en utf8"
else
echo "fichier non utf8"
# sauvegarde du fichier source
mv $filename $filename.save
# ecriture du fichier encode
iconv -f $encodeFrom -t $encodeTo $filename.save -o $filename
# si l'on veut supprimer les sauvegardes
rm $filename.save
fi
done