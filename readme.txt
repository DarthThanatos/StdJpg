1.Włącz Windowsa, bo tam wszystko działa (na Ubuntu szkoda czasu)
2.Otwórz konsolę cmd.
3.odpalasz make => wynik domyślny: jpg_decompresser.exe
4.odpalasz jpg_decompresser.exe plik.jpg => wynik domyślny: result.txt. 
Domyślnie po odpaleniu jpg_decompresser.exe musisz dwa razy klepnąć enter, bo 
po drodze dałem dwa getchary() w main(). Pojawią się dwie tabelki a po nich 
napis "Done". Wtedy wiesz, że poszło dobrze. Jak chcesz to możesz to wypisywanie 
i getchary powywalać.
5.odpalasz display_jpg.py plik_pierwotny.jpg
Plik "plik_pierwotny.jpg" służy do porównania pomiędzy naszym dekompresorem 
a dekompresorem np. funkcji imread(file) - po odpaleniu display_jpg.py jeden 
za drugim wyskoczą dwa okienka - pierwszy będzie wynikiem naszego parsera, 
a drugi wynikiem imread.
 
Wszystko rób w tym folderze. Spróbuj poodpalać dla obrazków, które dodałem.
Polecam zacząć od tortoise.jpg bo to chyba najbardziej wdzięczny obrazek z nich wszystkich.
Jak będziesz chciał, to po tym wszystkim w katalogu pojawi się plik abc.txt - tam jest sparsowana 
metadata z jako takim opisem poszczególnych nagłówków jpga.