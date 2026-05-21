# LEIAME — Trabalho I: Escalonador com Classes de Prioridade por Loteria

Disciplina: Sistemas Operacionais  
Base: xv6-riscv  
Alunos: Matheus Zandoná e Luis Scalvi

---

## 1. Descrição do Trabalho

Implementação de um escalonador de processos com **classes de prioridade baseadas em loteria**, contemplando 4 classes:

- Classe 0 — maior prioridade — 6 bilhetes
- Classe 1 — 3 bilhetes
- Classe 2 — 2 bilhetes
- Classe 3 — menor prioridade — 1 bilhete

Total: 12 bilhetes, gerando as seguintes probabilidades teóricas:

| Classe | Bilhetes | Probabilidade |
|--------|----------|---------------|
| 0      | 6        | 50,00%        |
| 1      | 3        | 25,00%        |
| 2      | 2        | 16,67%        |
| 3      | 1        |  8,33%        |

A seleção da classe é feita por loteria e, dentro de cada classe, os processos são escalonados em round-robin.

---

## 2. Arquivos Modificados

kernel/param.h   — adição da constante NUMCLASS 4
kernel/proc.h    — adição dos campos class e scheduled_count na struct proc
kernel/proc.c    — implementação do escalonador (loteria + round-robin + contagem)
e modificação da função kfork para passar parâmetro
kernel/syscall.c — mapeamento da syscall fork atualizado
kernel/sysproc.c — sys_fork atualizado para receber parâmetro de classe
kernel/defs.h    — assinatura atualizada de kfork(int class)
user/teste.c     — programa de teste para criar processos em classes selecionadas

---

## 3. Detalhes da Implementação

3.1 "kernel/param.h"
Adicionada a constante:

#define NUMCLASS 4  // número de classes do escalonador

3.2 "kernel/proc.h"
Adicionados dois campos na "struct proc":

int class;            // classe de prioridade do processo (0 a 3)

3.3 "kernel/proc.c" — função "sort_class"
Implementa a loteria com 12 bilhetes:

int sort_class(void) {
    int r = rand() % 12;  // sorteia número de 0 a 11

    if (r < 6)       return 0;  // bilhetes 0-5  → 6/12 = 50,00%
    else if (r < 9)  return 1;  // bilhetes 6-8  → 3/12 = 25,00%
    else if (r < 11) return 2;  // bilhetes 9-10 → 2/12 = 16,67%
    else             return 3;  // bilhete  11   → 1/12 =  8,33%
}


3.4 "kernel/proc.c" — função "scheduler"
Após sortear a classe, aplica round-robin dentro dela usando "last_index_class[]":

static int last_index_class[NUMCLASS];

void scheduler(void) {
    ...
    int class = sort_class();  // loteria
    for (int i = 0; i < NPROC; i++) {
        int idx = (last_index_class[class] + i) % NPROC;  // round-robin
        p = &proc[idx];
        acquire(&p->lock);
        if (p->state == RUNNABLE && p->class == class) {
            p->state = RUNNING;
            p->scheduled_count++;
            total_scheduled++;
            class_scheduled[class]++;
            ...
            last_index_class[class] = (idx + 1) % NPROC;  // avança RR
        }
    }
}

3.5 "kernel/syscall.c" — tabela de syscalls
Mantém o mapeamento da syscall "fork" que agora aceita o parâmetro de classe:

// Protótipo da syscall fork
extern uint64 sys_fork(void);

// Tabela de mapeamento número → função
static uint64 (*syscalls[])(void) = {
[SYS_fork]    sys_fork,   // fork agora passa classe para kfork
...
};

3.6 "kernel/sysproc.c" — função "sys_fork"
Atualizada para receber o parâmetro de classe enviado pelo processo de usuário:

uint64
sys_fork(void)
{
  int class;
  argint(0, &class);  // lê o parâmetro classe passado pelo usuário (registrador a0)
  return kfork(class);
}

3.7 "kernel/proc.c" — função "kfork"
Recebe a classe como parâmetro e a atribui ao processo filho:

int kfork(int class) {
    ...
    np->class = class;
    np->state = RUNNABLE;
    ...
}

3.8 "user/teste.c"
Programa de teste que cria um processo filho em loop infinito na classe especificada:

#include "kernel/stat.h"
#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char* argv[]) {

    if(strlen(argv[1]) == 0){
        printf("invalid argument\n");
        exit(0);
    }
    char* num = argv[1];
    int class = 0;

    if(num[0] == '-'){
        if((strlen(argv[1]) >= 2) && num[1] >= '0' && num[1] <= '9'){
            for(int i = 1; i < strlen(argv[1]); i++){
                class *= 10;
                class += argv[1][i] - '0';
            }
        } else {
            printf("invalid argument\n");
            exit(0);
        }
    } else {
        if(num[0] >= '0' && num[0] <= '9'){
            for(int i = 0; i < strlen(argv[1]); i++){
                class *= 10;
                class += argv[1][i] - '0';
            }
        } else {
            printf("invalid argument\n");
            exit(0);
        }
    }

    class %= 4;
    int pid = fork(class);

    if (pid == 0) {
        while (1);  // filho permanece em execução
    }
    exit(0);
}

---

## 4. Pré-requisitos

- Sistema Linux (ou WSL no Windows)
- QEMU instalado
- Toolchain RISC-V: "riscv64-linux-gnu-gcc"

Para instalar no Ubuntu/Debian:
sudo apt update
sudo apt install qemu-system-riscv64 gcc-riscv64-linux-gnu

---

## 5. Como Compilar e Executar

5.1 Clonar o repositório
git clone https://github.com/MatheusCZandona/XV6-scheduler-activity.git
cd XV6-scheduler-activity

5.2 Compilar e iniciar o xv6
make qemu

Aguarde aparecer o prompt do shell do xv6:
init: starting sh
$

---

## 6. Executando os Testes

6.1 Teste básico — um processo por classe

No prompt escreva o comando {ls} para listar os programas disponíveis
$ ls
Dentre os programas estará o programa {teste}
para chamar o programa teste, basta escrever teste e em seguida passar
o numero (de 0 a 3) representando a classe que você deseja
$ teste 0
$ teste 1
$ teste 2
$ teste 3


6.2 Teste recomendado — três processos por classe

Para resultados mais precisos (o xv6 roda com 3 CPUs simultâneos), crie 3 
instâncias de cada classe:

$ teste 0 & teste 0 & teste 0 &
$ teste 1 & teste 1 & teste 1 &
$ teste 2 & teste 2 & teste 2 &
$ teste 3 & teste 3 & teste 3 &

6.3 Visualizar as estatísticas

Aguarde alguns segundos e pressione Ctrl+P para exibir o procdump com as estatísticas:

--- Estatisticas por Classe (total: XXXX) ---
Classe  Escalonado    Porcentagem
------  ----------    -----------
0       XXXX          ~50%
1       XXXX          ~25%
2       XXXX          ~16%
3       XXXX          ~8%
---------------------------------------------

Observação: Quanto maior o total de escalonamentos, mais as porcentagens 
se aproximam dos valores teóricos (lei dos grandes números).
Recomenda-se aguardar um total acima de 5000 escalonamentos para resultados representativos.

6.4 Verificar a classe de cada processo

O "procdump" (Ctrl+P) também exibe a lista de processos com sua respectiva classe:
"""
PID  STATE     NAME    CLASS
1    sleep     init    0
2    sleep     sh      0
5    running   teste   0
8    running   teste   1
11   running   teste   2
14   runnable  teste   3

## 7. Resultado Esperado

Com um número suficiente de escalonamentos, as porcentagens obtidas devem convergir para:

| Classe | Esperado | Obtido (aproximado) |
|--------|----------|---------------------|
| 0      | 50,00%   | ~48–52%             |
| 1      | 25,00%   | ~23–27%             |
| 2      | 16,67%   | ~15–18%             |
| 3      |  8,33%   | ~7–10%              |

Pequenas variações são normais devido à natureza probabilística da loteria,
feita com numeros pseudo-aleatórios.