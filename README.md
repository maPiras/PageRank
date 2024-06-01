# Progetto Pagerank - Laboratorio II 2024

La fase del calcolo del pagerank si divide in due step: il primo è la consumazione degli indici da parte dei thread (in maniera atomica) la quale avviene per mezzo di una condition variable la quale permette di segnalare quando un thread ha prelevato l'indice, per permettere ai successivi di procedere anch'essi con la consumazione.
Il secondo step consiste nel calcolo di delle componenti di xnext da parte dei singoli thread, i quali concluso il calcolo 
