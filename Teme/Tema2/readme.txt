Copyright Radu-Andrei Dumitrescu 322CA 2021

Tema2 PCOM - creare de server care realizeaza comunicatia intre un sender de UDP si mai multi clienti print TCP

Timp de lucru: aproximativ 6h pe parcursul a 4 zile

Server:
     - am folosit FD pentru operatiile cu socketi
     - am impartit logica in 4 zone
        - FD pe STDIN unde puteam primi exit
        - FD pentru senderul de UDP
            - aici primeam un pachet standard pe care il parsam
            - verificam daca am clienti catre care trebuie sa trimit pachetul si daca am clienti
              pentru care sa pastrez pachetul in coada
        - socket liber pentru client nou
            - aici am implementat o logica de verifica daca e client nou,
              daca nu era si exista deja ID-ul ii trimiteam mesajul ";ID_EXISTS;\n",
            - daca nu era client nou, dar nu era conectat deja faceam update in dataset cu noile info despre el
        - socket de receive pentru fiecare client conectat pe TCP
            - puteam primi mesaj de subscribe, unsubscribe si beacon de disconnnect
        
Subscriber:
    - se conecteaza la server
    - isi trimite id-ul la conectare
    - asteapta comenzi de la stdin cu subscribe / unsubscribe sau exit
        - trimite mesajul mai departe pe tcp
    - pe socketul de TCP am implementat un protocol care citeste date pana la '\n'
    - delimitatorul de pachete fiind '\n'
    - flagurile de err si exit le-am delimitat astfel: ";{flag};"


Probleme intampinate:
    - fara sa dezactivez bufferul de std nu am reusit in niciun fel sa fac checkerul sa citeasca de la stdout
    - pe quick flow nu reuseam sa fac delimitarea intre pachete

Feedback:
    - foarte faina tema, mi s-a parut challenging ca sa invatam sa lucram cat mai bine cu socketi
