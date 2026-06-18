# LEIAME — Trabalho I: Escalonador Stride Scheduling

Disciplina: Sistemas Operacionais
Base: xv6-riscv
Alunos: Matheus Zandoná e Luis Scalvi

---

## 1. Descrição do Trabalho

Implementação do escalonador de processos **Stride Scheduling** (escalonamento
em passos largos). Cada processo recebe um número fixo de bilhetes (tickets).
O "passo" (stride) de cada processo é calculado como:

    stride = DIVSTRIDE / tickets   (DIVSTRIDE = 10.000)

Cada processo inicia com passada zero (pass = 0). O escalonador sempre
seleciona o processo RUNNABLE com o menor valor de passada atual. Em caso de
empate, o processo com o maior PID é selecionado. Após selecionado, a passada
do processo é incrementada pelo seu stride:

    pass += stride

Essa abordagem é determinística: processos com mais tickets têm stride menor,
são selecionados com mais frequência e, portanto, recebem maior fração da CPU
proporcionalmente aos seus bilhetes.

Exemplo com 3 processos (enunciado):
    A: 100 tickets → stride 100
    B:  50 tickets → stride 200
    C: 250 tickets → stride  40

Proporção de CPU esperada:
    A ~ 25%   (100 de 400 bilhetes totais)
    B ~ 12%   ( 50 de 400 bilhetes totais)
    C ~ 62%   (250 de 400 bilhetes totais)

---

## 2. Arquivos Modificados

kernel/param.h   — adição das constantes NUMCLASS e DIVSTRIDE
kernel/proc.h    — adição dos campos tickets, pass e scheduled_count na struct proc
kernel/proc.c    — implementação do stride scheduler (stride_pick + scheduler)
                   modificação de kfork para receber tickets como parâmetro
                   modificação de allocproc e freeproc para inicializar/limpar campos
                   modificação de procdump para exibir tabela formatada com CPU%
kernel/sysproc.c — sys_fork atualizado para receber parâmetro de tickets
kernel/defs.h    — assinatura atualizada de kfork(int tickets)
user/user.h      — assinatura atualizada de fork(int tickets)
user/init.c      — fork atualizado para passar 1000 tickets ao shell
user/sh.c        — fork1() atualizado para usar fork(1000)
user/grind.c     — fork() atualizado para usar fork(1000)
user/logstress.c — fork() atualizado para usar fork(1000)
user/stressfs.c  — fork() atualizado para usar fork(1000)
user/teste.c     — programa de teste: cria filho com N tickets em loop infinito
user/stridetest.c— programa de teste automático: cria processos A(100), B(50), C(250)
user/run.c       — utilitário: executa um programa como filho com N tickets definidos

---

## 3. Detalhes da Implementação

3.1 kernel/param.h
Adicionadas as constantes:

    #define DIVSTRIDE 10000  // constante divisora para cálculo do stride


3.2 kernel/proc.h
Adicionados três campos na struct proc:

    int    tickets;         // número de bilhetes do processo
    uint64 pass;            // passada acumulada atual
    int    scheduled_count; // quantas vezes o processo foi escalonado

Adicionada também a struct procinfo para exportação de dados ao userspace:

    struct procinfo {
      int    pid;
      int    tickets;
      int    stride;
      uint64 pass;
      int    sched;
      int    state;
      char   name[16];
    };


3.3 kernel/proc.c — função stride_pick
Percorre a tabela de processos e retorna o processo RUNNABLE com menor passada.
Desempate pelo maior PID. Retorna com p->lock já adquirido:

    static struct proc*
    stride_pick(void)
    {
      struct proc *selected = 0;
      uint64 min_pass = (uint64)-1;

      for(struct proc *p = proc; p < &proc[NPROC]; p++){
        acquire(&p->lock);
        if(p->state != RUNNABLE){ release(&p->lock); continue; }
        if(p->pass < min_pass ||
           (p->pass == min_pass && selected != 0 && p->pid > selected->pid)){
          if(selected != 0) release(&selected->lock);
          min_pass = p->pass;
          selected = p;
        } else {
          release(&p->lock);
        }
      }
      return selected;
    }


3.4 kernel/proc.c — função scheduler
Chama stride_pick(), atualiza a passada e escalona o processo:

    void
    scheduler(void)
    {
      struct proc *p;
      struct cpu *c = mycpu();
      c->proc = 0;
      for(;;){
        intr_on();
        intr_off();
        p = stride_pick();
        if(p != 0){
          p->pass += DIVSTRIDE / p->tickets;  // incrementa passada
          p->state = RUNNING;
          p->scheduled_count++;
          total_scheduled++;
          c->proc = p;
          swtch(&c->context, &p->context);
          c->proc = 0;
          release(&p->lock);
        }
      }
    }

Nota sobre multicore: com CPUS > 1, o scheduler de cada CPU opera de forma
independente. Para resultados determinísticos e proporções exatas, recomenda-se
executar com CPUS=1:

    make qemu CPUS=1


3.5 kernel/proc.c — função allocproc
Inicializa os novos campos ao alocar um processo:

    p->tickets         = 100;  // padrão: 100 tickets
    p->pass            = 0;    // passada inicial zero
    p->scheduled_count = 0;    // contador zerado


3.6 kernel/proc.c — função freeproc
Limpa os campos ao liberar um processo:

    p->tickets         = 0;
    p->pass            = 0;
    p->scheduled_count = 0;


3.7 kernel/proc.c — função kfork
Recebe tickets como parâmetro e os atribui ao processo filho:

    int kfork(int tickets)
    {
      ...
      if(tickets <= 0) tickets = 100;  // garante valor válido
      np->tickets         = tickets;
      np->pass            = 0;
      np->scheduled_count = 0;
      np->state           = RUNNABLE;
      ...
    }


3.8 kernel/proc.c — função procdump (Ctrl+P)
Exibe tabela formatada com bordas, incluindo stride, passada e porcentagem de CPU:

    +-----+------------+----------+------+-------+--------+-------+------+
    | PID | NOME       | ESTADO   |  TKT | STRIDE|  PASS  | SCHED | CPU% |
    +-----+------------+----------+------+-------+--------+-------+------+
    | 1   | init       | SLEEP    | 100  | 100   | 2600   | 26    | 5%   |
    | 2   | sh         | SLEEP    | 1000 | 10    | 220    | 22    | 4%   |
    | 6   | teste      | RUNNING  | 250  | 40    | 7800   | 195   | 62%  |
    | 10  | teste      | RUNNABLE | 100  | 100   | 7800   | 78    | 25%  |
    | 14  | teste      | RUNNABLE | 50   | 200   | 7800   | 37    | 12%  |
    +-----+------------+----------+------+-------+--------+-------+------+
      total escalonamentos: 296
    +-----+------------+----------+------+-------+--------+-------+------+

Colunas:
    TKT    — tickets do processo
    STRIDE — passo = 10000 / tickets
    PASS   — passada acumulada (quanto maior, menos será escalonado)
    SCHED  — número de vezes que o processo foi escalonado
    CPU%   — porcentagem real de CPU = (sched / total) * 100


3.9 kernel/sysproc.c — função sys_fork
Atualizada para ler o parâmetro tickets do registrador a0:

    uint64
    sys_fork(void)
    {
      int tickets;
      argint(0, &tickets);
      return kfork(tickets);
    }


3.10 user/user.h
Assinatura da syscall fork atualizada:

    int fork(int);  // parâmetro: número de tickets do filho


3.11 user/init.c
O processo init cria o shell com 1000 tickets para garantir boa responsividade:

    pid = fork(1000);


3.12 user/sh.c
A função fork1() usa 1000 tickets para processos criados pelo shell:

    int fork1(void) {
      int pid = fork(1000);
      if(pid == -1) panic("fork");
      return pid;
    }


3.13 user/teste.c
Programa de teste manual. Recebe o número de tickets como argumento,
cria um filho em loop infinito com esses tickets e aguarda:

    $ teste 250     → cria filho com 250 tickets
    $ teste 100     → cria filho com 100 tickets
    $ teste 50      → cria filho com 50 tickets


3.14 user/stridetest.c
Programa de teste automático que reproduz o exemplo do enunciado.
Cria 3 filhos simultaneamente (A=100, B=50, C=250 tickets), cada um
executando 50.000.000 iterações, e aguarda todos terminarem:

    $ stridetest

Saída esperada (ordem de conclusão proporcional aos tickets):
    Processo C (tickets=250): concluido primeiro
    Processo A (tickets=100): concluido segundo
    Processo B (tickets=50):  concluido por último


3.15 user/run.c
Utilitário que permite executar qualquer programa como filho com tickets
definidos pelo usuário. Útil para rodar processos em background com tickets
customizados sem depender do fork1() do shell (que usa 1000 fixo):

    $ run 250 teste 250 &
    $ run 100 teste 100 &
    $ run 50  teste 50  &

Implementação:
    int main(int argc, char *argv[]) {
      // argv[1] = tickets, argv[2] = programa, argv[3+] = args
      int tickets = atoi(argv[1]);
      int pid = fork(tickets);
      if(pid == 0){
        exec(argv[2], argv + 2);
      }
      wait(0);
      exit(0);
    }

---

## 4. Pré-requisitos

- Sistema Linux (ou WSL no Windows)
- QEMU instalado
- Toolchain RISC-V: riscv64-linux-gnu-gcc

Para instalar no Ubuntu/Debian:

    sudo apt update
    sudo apt install qemu-system-riscv64 gcc-riscv64-linux-gnu

---

## 5. Como Compilar e Executar

5.1 Compilar e iniciar o xv6 (recomendado: 1 CPU para resultado determinístico)

    make qemu CPUS=1

Aguarde aparecer o prompt do shell do xv6:

    init: starting sh
    $

5.2 Para executar com múltiplas CPUs (comportamento aproximado)

    make qemu

---

## 6. Executando os Testes

6.1 Teste automático — exemplo do enunciado (A=100, B=50, C=250 tickets)

    $ stridetest

Aguarde a conclusão. A ordem de término reflete a proporção de CPU recebida.


6.2 Teste manual — processos em background com tickets customizados

    $ run 250 teste 250 &
    $ run 100 teste 100 &
    $ run 50  teste 50  &

Aguarde alguns segundos e pressione Ctrl+P para exibir as estatísticas.


6.3 Visualizar estatísticas (Ctrl+P)

Pressione Ctrl+P no console do xv6. Exemplo de saída esperada com CPUS=1:

    +-----+------------+----------+------+-------+--------+-------+------+
    | PID | NOME       | ESTADO   |  TKT | STRIDE|  PASS  | SCHED | CPU% |
    +-----+------------+----------+------+-------+--------+-------+------+
    | 1   | init       | SLEEP    | 100  | 100   | 2600   | 26    | 5%   |
    | 2   | sh         | SLEEP    | 1000 | 10    | 220    | 22    | 4%   |
    | 6   | teste      | RUNNING  | 250  | 40    | 7360   | 184   | 62%  |
    | 10  | teste      | RUNNABLE | 100  | 100   | 7400   | 74    | 25%  |
    | 14  | teste      | RUNNABLE | 50   | 200   | 7400   | 37    | 12%  |
    +-----+------------+----------+------+-------+--------+-------+------+
      total escalonamentos: 296
    +-----+------------+----------+------+-------+--------+-------+------+

Observação importante sobre as passadas (PASS):
    Os três processos acumulam passadas próximas ao mesmo valor, o que é a
    propriedade matemática central do stride scheduler: as passadas convergem,
    garantindo que cada processo receba exatamente a fração de CPU proporcional
    aos seus tickets a longo prazo.

---

## 7. Resultado Esperado

Com CPUS=1 e número suficiente de escalonamentos, as porcentagens obtidas
devem convergir para os valores teóricos:

    | Processo | Tickets | Stride | Esperado | Obtido (aprox.) |
    |----------|---------|--------|----------|-----------------|
    | C        | 250     | 40     | 62,50%   | ~61–63%         |
    | A        | 100     | 100    | 25,00%   | ~24–26%         |
    | B        | 50      | 200    | 12,50%   | ~11–13%         |

Com CPUS > 1: cada CPU executa seu próprio scheduler() de forma independente,
portanto dois núcleos podem escalonar processos simultaneamente, o que reduz o
determinismo. As proporções ainda refletem os tickets, mas com menor precisão.
Recomenda-se CPUS=1 para demonstrações formais.

Propriedade de convergência das passadas (verificável no Ctrl+P):
    pass(C) = sched(C) * 40   ≈ pass(A) = sched(A) * 100   ≈ pass(B) = sched(B) * 200