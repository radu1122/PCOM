Copyright Radu-Andrei Dumitrescu 322CA 2021

Folosire client: ./client <id> <port>
Folosire server: ./server <port>

Am implementat toate functionalitatile.

Client:
    - pot exista oricat de multi clienti
    - nu pot exista 2 clienti cu acelasi id in acelasi Timp
    - daca un client se deconecteaza si apoi se reconecteaza suma de bani ramane neschimbata
    - nu se pot trimite bani unui id care nu se exista
    - nu se pot trimite bani id-ului tau
    - toate functionalitatile merg corect
    - am implementat si verificari pentru input invalid

Server:
    - primeste de la client comenzile
    - proceseaza datele care sunt sub forma unui haspMap cu key ID si value un struct care contine isConnected si suma de bani a acelui client
    - trimite catre server mesaje clare de response:
        - ;USER_EXISTS; - daca userul este deja conectat
        - ;MONEY_SENT; - daca banii au fost trimisi cu success
        - ;GET_ERROR; - daca suma de bani ceruta este prea mare
        - ;GET_MONEY <sum> - daca banii au fost extrasi cu succes si ce suma
        - ;EXIT; - nu am reusit sa implementez la nivel de server sa se inchida clientul o data cu serverul
        - ;SHOW_SUM <sum> - trimite catre client suma de bani actuala din cont
    - el primeste comenzile de la client serializat asa cum sunt primite de la input de client + ID client care trimite

Surse de inspiratie: 
    - tema 2 la PCOM a mea: https://github.com/radu1122/PCOM/blob/master/Teme/Tema2/server.cpp
        - daca este nevoie pot oferi unui asistent acces la git pentru a confirma
    - https://www.geeksforgeeks.org/udp-client-server-using-connect-c-implementation/
        - aveam un bug ca serverul nu trimitea mesaje catre client, desi clientul trimitea catre server ok 
        - m-am folosit de aceasta sursa de inspiratie ca sa vad ca greseam un bind pe Server

Feedback:
    - destul de challenging sa implementez in 2 ore functionalitatile complete

PS: ma astept sa mai fie diverse edge caseuri sau inputuri invalide pe care nu le-am luat in calcul