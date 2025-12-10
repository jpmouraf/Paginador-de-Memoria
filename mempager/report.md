<!-- LTeX: language=pt-BR -->

# PAGINADOR DE MEMÓRIA -- RELATÓRIO

1. Termo de compromisso

   Ao entregar este documento preenchiso, os membros do grupo afirmam que todo o código desenvolvido para este trabalho é de autoria própria. Exceto pelo material listado no item 3 deste relatório, os membros do grupo afirmam não ter copiado material da Internet nem ter obtido código de terceiros.

2. Membros do grupo e alocação de esforço

   Preencha as linhas abaixo com o nome e o email dos integrantes do grupo. Substitua marcadores `XX` pela contribuição de cada membro do grupo no desenvolvimento do trabalho (os valores devem somar 100%).

   - João Paulo Moura Furtado <joaopmfurtado@ufmg.br> 50%
   - Túlio Henrique Rodrigues Costa <tuliohrc@ufmg.br> 50%

3. Referências bibliográficas

   - Livro "Operating Systems: Three Easy Pieces": https://pages.cs.wisc.edu/~remzi/OSTEP/

4. Detalhes de implementação

   1. Descreva e justifique as estruturas de dados utilizadas em sua solução.

      Temos 3 estruturas de dados principais:

      - `Page`: A página contendo as informações relativas a mesma, com bits de validade, on_disk, frame respectivo, bloco respectivo, presente, prot (permissões de acesso) e dirty (modificado). Representando uma página virtual no espaço de memória de um processo.
      - `Process`: Com o PID, uma lista de paginas e o número de páginas. Representa o processo e sua tabela de páginas, aumentando o seu número de páginas com o `page_extend()`.
      - `Frame`: Com um booleano de is_free, o PID responsável pelo frame e a página do mesmo. Representando o frame de memória física, usando uma tabela de páginas invertida.

      Além disso tempos algumas variaveis globais como:

      - `processes` que é um array de processos.
      - `num_processes` e `max_processes`.
      - `frame_table` que é um array de frames da memória física.
      - `n_frames_global` e `clock_hand` utilizados nos algoritmos de paginação.
      - `disk_blocks` bitmap de blocos do disco.
      - `n_blocks_global`.
      - `lock` utilizado para evitar condições de corrida nas variaveis do sistema de paginação.

   2. Descreva o mecanismo utilizado para controle de acesso e modificação às páginas.

      O algoritmo utiliza bits de proteção combinados com o algoritmo Second-chance (clock algorithm) para gerir os acessos e controlar o paginamento. Nós temos 2 casos de acesso a memória em caso de PAGE_FAULT:

      - Primeiro acesso (tabela não está cheia) -> página carregada após page fault (present = 0 na página em questão), dirty = 0, prot = PROT_READ, procurando por frame (quadro) livre na memória.
      - Second-chance (tabela cheia) -> quando o ponteiro do clock chega a uma página, o prot é rebaixado a PROT_NONE, e se já estava com este valor, é paginado para o disco.

      E se PAGE_HIT:

      - Tentativa de escrita -> prot atualizado para PROT_READ | PROT_WRITE
      - Tentativa de leitura -> prot atualizado para PROT_READ

5. Melhorias na Documentação

   1. Ambiguidade no Adiamento de Trabalho

      A especificação pede adiamento de trabalho e diz para adiar alocações o máximo possível. Porém, também exige que pager_extend retorne NULL imediatamente se não houver blocos de disco. Nós falhamos no Teste 10 inicialmente porque tentamos adiar a escolha do bloco de disco. Descobrimos que a reserva do slot no disco não pode ser adiada, apenas a alocação do frame de memória.
      Com isso, a seção Adiamento de Trabalho poderia ser mais explícita quanto à exceção da reserva de disco. Deveria esclarecer que, embora a alocação de memória física deva ser tardia, a reserva lógica do bloco de disco deve ocorrer imediatamente no pager_extend.

   2. Efeitos colaterais do pager_syslog

      O texto diz que pager_syslog deve se comportar "como se estivesse fazendo leitura". Isso é vago quanto ao impacto no algoritmo de substituição de páginas. Descobrimos que a leitura feita pelo syslog deve contar como uma referência para o algoritmo de Segunda Chance, reativando a permissão PROT_READ se a página estava marcada para sair. Se isso não for feito, o syslog pode ler uma página e ela ser expulsa logo em seguida, o que contradiz a lógica de recentemente usada.

      Com isso, seria útil esclarecer que os acessos realizados via pager_syslog devem influenciar o algoritmo de substituição de
      páginas, resetando o bit de referência/segunda chance, já que contam como um acesso válido à memória do processo.
