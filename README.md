# Tema 2: Load Balancer

### Nume: Potoceanu Ana-Maria  

---

### Descriere

Pentru realizarea unui Load Balancer, am implementat prima dată funcționalitățile serverului. Pentru fiecare server, vom reține hashtable-ul și ID-ul corespunzător. Am utilizat hashtable-ul care a fost pus la dispoziție în cadrul laboratorului. Prima dată, am creat funcția de inițializare a unui server. Pentru hashtable-ul dintr-un server, ne folosim de funcția `ht_create`. Stocarea datelor într-un hashtable o facem cu ajutorul funcției `ht_put`. Pentru aflarea valorii asociate unei chei am folosit `ht_get`, iar pentru ștergerea unei chei am utilizat `ht_remove_entry`. În final, am eliberat memoria pentru întregul server.

Pentru implementarea unui Load Balancer am folosit un vector care va conține toate etichetele, sortate după hash-ul etichetelor. Am numit acest vector `hashring`. Vectorul trebuie menținut sortat pentru a putea face căutare binară, fiind considerată o metodă mai eficientă. Am creat și un vector de pointeri către serverele principale, care nu va conține și celelalte două replici. Am reținut, de asemenea, numărul de elemente din `hashring` și numărul de servere.

Funcția `loader_store` stochează un obiect pe unul dintre serverele disponibile. Inițial parcurgem `hashring`, verificăm unde trebuie adăugat obiectul în funcție de hash-ul lui, aflăm poziția serverului în care trebuie adăugat și în final îl punem în serverul dorit, folosind `server_store`. Dacă obiectul nu a fost încă adăugat, îl vom pune în serverul care corespunde primei etichete din `hashring`.

În funcția `loader_retrieve` aflăm pe ce server se află cheia pe care o avem și îi extragem valoarea. Aici am abordat din nou două cazuri, la fel ca în funcția `loader_store`.

În cadrul funcției `loader_add_server`, prima dată am realocat vectorul `hashring` și cel cu servere, deoarece, odată cu adăugarea unui nou server, se vor adăuga în `hashring` cele trei etichete corespunzătoare. Pentru adăugarea unui element în `hashring` am folosit funcția `put_hashring`. Dacă nu avem niciun element în vector, atunci vom adăuga pe prima poziție. Trebuie să adăugăm etichetele în funcție de hash, pentru a menține vectorul sortat. Inițial, am tratat cazul când trebuie să punem pe prima poziție, mai exact dacă hash-ul etichetei pe care vrem să o adăugăm este mai mic decât hash-ul primei etichete din `hashring`. Mai departe, avem cazul când vom adăuga în interiorul `hashring`. Verificăm unde trebuie să adăugăm eticheta și vom muta toate elementele cu o poziție la dreapta, pentru a face loc etichetei. Dacă eticheta nu a fost încă adăugată, înseamnă că va fi pusă pe ultima poziție. De fiecare dată când adăugăm un server trebuie să facem redistribuirea elementelor. În funcția `map`, folosim căutarea binară pentru a găsi în `hashring` poziția etichetei. Vom reține de asemenea și următoarea poziție pentru a putea calcula ID-ul corespunzător. Trebuie să facem verificarea să nu avem o replică, deoarece redistribuirea se face către un server nou. Parcurgem hashtable-ul și mutăm elementele din server vechi în cel nou.

Funcția `loader_remove_server` șterge un server. Atunci când ștergem un server, va trebui să eliminăm toate replicile sale din `hashring`. Acest lucru l-am făcut cu ajutorul funcției `delete_hashring`. Căutăm poziția pe care se află eticheta dată, folosind căutare binară și mutăm elementele cu o poziție la stânga. După ce se șterge un server din sistem, trebuie să redistribuim elementele din el, astfel am implementat funcția `map_delete`. Pentru început, aflăm poziția din vectorul de servere (pe care se află serverul cu ID-ul dat). Parcurgem întregul hashtable și stocăm elementele rămase pe serverele disponibile. În final, eliberăm memoria pentru serverul care urmează a fi șters și mutăm elementele din vectorul de servere cu o poziție la stânga.

În final, eliberăm memoria pentru `load_balancer`. Prima dată, eliberăm vectorul cu etichete, apoi parcurgem vectorul de servere și dealocăm memoria pentru fiecare server în parte.
