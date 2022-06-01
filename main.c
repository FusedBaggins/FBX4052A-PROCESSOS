#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#pragma pack(1)

#define DEF_TAMANHO_MASCARA 7
#define DEF_NRO_PROCESSOS 4

struct cabecalho
{
    unsigned short tipo;
    unsigned int tamanho_arquivo;
    unsigned short reservado1;
    unsigned short reservado2;
    unsigned int offset;
    unsigned int tamanho_image_header;
    int largura; // É o número de colunas da matriz;
    int altura;  // É o número de linhas da matriz
    unsigned short planos;
    unsigned short bits_por_pixel;
    unsigned int compressao;
    unsigned int tamanho_imagem;
    int largura_resolucao;
    int altura_resolucao;
    unsigned int numero_cores;
    unsigned int cores_importantes;
};
typedef struct cabecalho CABECALHO;

struct rgb
{
    unsigned char blue;
    unsigned char green;
    unsigned char red;
};
typedef struct rgb RGB;

/*Algorítmo de ordenação*/
void QuickSort(int *a, int left, int right)
{
    int i, j, x, y;

    i = left;
    j = right;
    x = a[(left + right) / 2];

    while (i <= j)
    {
        while (a[i] < x && i < right)
        {
            i++;
        }
        while (a[j] > x && j > left)
        {
            j--;
        }
        if (i <= j)
        {
            y = a[i];
            a[i] = a[j];
            a[j] = y;
            i++;
            j--;
        }
    }

    if (j > left)
    {
        QuickSort(a, left, j);
    }
    if (i < right)
    {
        QuickSort(a, i, right);
    }
}

int alinhar(int largura)
{
    return (largura * 3) % 4;
}

void percorrer_imagem(int altura, int largura, RGB *valores, FILE *arq_entrada, FILE *arq_saida)
{
    char c;
    int i, j;
    for (i = 0; i < altura; i++)
    {
        /*Alinhamento*/
        int alinhamento = alinhar(largura);

        if (alinhamento)
        {
            alinhamento = 4 - alinhamento;
        }

        /*Leitura*/
        for (j = 0; j < largura; j++)
        {
            fread(&valores[i * largura + j], sizeof(RGB), 1, arq_entrada);
        }

        /*Alinhamento*/
        for (j = 0; j < alinhamento; j++)
        {
            fread(&c, sizeof(unsigned char), 1, arq_entrada);
            fwrite(&c, sizeof(unsigned char), 1, arq_saida);
        }
    }
}

void aplicar_filtro(int valor_inicial, int id_processo, int altura, int largura, RGB *valores, FILE *arq_entrada, FILE *arq_saida, RGB *valores_saida)
{

    /*FIltros de tamanho 3, por exemplo, tem 9 posições (3 linhas e 3 colunas). Tamanho dele é igual a 3*3.*/
    int azul[DEF_TAMANHO_MASCARA * DEF_TAMANHO_MASCARA];
    int verde[DEF_TAMANHO_MASCARA * DEF_TAMANHO_MASCARA];
    int vermelho[DEF_TAMANHO_MASCARA * DEF_TAMANHO_MASCARA];

    int metade_mascara = DEF_TAMANHO_MASCARA / 2;
    int cont_filtro, i = 0, j = 0, ii = 0, jj = 0;
    RGB pixel;

    /*Vai da posião inicial passada como parâMetro até o final do intervalo do processo, considerando o nro de processos definidos*/
    for (i = valor_inicial; i < altura / DEF_NRO_PROCESSOS * (id_processo + 1); i++)
    {
        for (j = 0; j < largura; j++)
        {
            cont_filtro = 0;

            int interromper = 0;

            /*Processo para aplicação do filtro mediana*/
            /*Inicia o contador na linha atual - a metade da mascara e termina na linha atual + a metade da mascara*/
            for (ii = i - metade_mascara; ii <= i + metade_mascara; ii++)
            {
                /*Caso algum dos valores da mediana tenha um valor inválido, quebra e printa o próprio pixel ao invés da mediana*/
                if (interromper)
                {
                    break;
                }

                /*Inicia o contador na coluna atual - a metade da mascara e termina na coluna atual + a metade da mascara*/
                for (jj = j - metade_mascara; jj <= j + metade_mascara; jj++)
                {
                    if ((ii >= 0) && (ii <= altura) && (jj >= 0) && (jj <= largura))
                    {
                        pixel = valores[ii * largura + jj];
                        /*Salva os valores nos respectivos arrays para ordenação e aplicação de mediana*/
                        azul[cont_filtro] = pixel.blue;
                        verde[cont_filtro] = pixel.green;
                        vermelho[cont_filtro] = pixel.red;
                        cont_filtro++;
                    }
                    /*Valor fora dos limites, interrompe*/
                    else
                    {
                        pixel = valores[i * largura + j];
                        interromper = 1;
                        break;
                    }
                }
            }

            if (!interromper)
            {
                /*Aplica ordenação para cada uma das componentes RGB*/
                QuickSort(azul, 0, (DEF_TAMANHO_MASCARA * DEF_TAMANHO_MASCARA) - 1);
                QuickSort(verde, 0, (DEF_TAMANHO_MASCARA * DEF_TAMANHO_MASCARA) - 1);
                QuickSort(vermelho, 0, (DEF_TAMANHO_MASCARA * DEF_TAMANHO_MASCARA) - 1);

                /*Pegando mediana e setando no pixel*/
                pixel.red = vermelho[(DEF_TAMANHO_MASCARA * DEF_TAMANHO_MASCARA) / 2];
                pixel.green = verde[(DEF_TAMANHO_MASCARA * DEF_TAMANHO_MASCARA) / 2];
                pixel.blue = azul[(DEF_TAMANHO_MASCARA * DEF_TAMANHO_MASCARA) / 2];
                valores_saida[i * largura + j] = pixel;
            }
            /*Valor fora dos limites*/
            else
            {
                valores_saida[i * largura + j] = pixel;
            }
        }
    }
}

void salvar_imagem(int valor_inicial, int id_processo, int altura, int largura, FILE *arq_saida, RGB *valor_saida)
{
    RGB pixel;
    char c;

    int i = 0, j = 0;
    for (i = 0; i < altura; i++)
    {
        /*Alinhamento*/
        int alinhamento = alinhar(largura); // 3 = nro bytes do pixel % 4 (pra fechar em imagens nao bagunçadas)

        if (alinhamento)
        {
            alinhamento = 4 - alinhamento;
        }

        /*Escrevendo no arquivo de saída*/
        for (j = 0; j < largura; j++)
        {
            pixel = valor_saida[i * largura + j];
            fwrite(&pixel, sizeof(RGB), 1, arq_saida);
        }

        for (j = 0; j < alinhamento; j++)
        {
            fwrite(&c, sizeof(unsigned char), 1, arq_saida);
        }
    }
}

int main(int argc)
{
    char entrada[100], saida[100];

    // Lendo qual arquivo de entrada e saída
    printf("Digite o nome do arquivo de entrada:\n");
    scanf("%s", entrada);

    printf("Digite o nome do arquivo de saida:\n");
    scanf("%s", saida);

    FILE *arquivo_saida, *arquivo_entrada;
    CABECALHO cabecalho;
    int shmid;

    arquivo_entrada = fopen(entrada, "rb");

    if (arquivo_entrada == NULL)
    {
        printf("Erro ao abrir o arquivo %s\n", entrada);
        exit(0);
    }

    arquivo_saida = fopen(saida, "wb");

    if (arquivo_saida == NULL)
    {
        printf("Erro ao abrir o arquivo %s\n", saida);
        exit(0);
    }

    // Le o cabecalho inteiro no arquivo de entrada
    fread(&cabecalho, sizeof(CABECALHO), 1, arquivo_entrada);

    // Escreve o cabecalho inteiro no arq de saída
    fwrite(&cabecalho, sizeof(CABECALHO), 1, arquivo_saida);

    // cria memória compartilhada
    shmid = shmget(5, cabecalho.altura * cabecalho.largura * sizeof(RGB), IPC_CREAT | 0600);

    // Associando a memória compartilhada no processo
    RGB *valores_saida = shmat(shmid, 0, 0);

    // Array onde será armazenada leitura das linhas
    RGB *valores = (RGB *)malloc(cabecalho.altura * cabecalho.largura * sizeof(RGB));

    // Lendo linhas
    // int largura, int altura, FILE *arq_entrada, FILE *arq_saida, RGB *valores
    // abrir_imagem(cabecalho.largura, cabecalho.altura, arquivo_entrada, arquivo_saida, vet);
    percorrer_imagem(cabecalho.altura, cabecalho.largura, valores, arquivo_entrada, arquivo_saida);

    // Criando as cópias de processo dinamicamente conforme o nro de processos
    int pid, id = 0, i;
    for (i = 1; i < DEF_NRO_PROCESSOS; i++)
    {
        pid = fork();
        if (pid == 0)
        {
            id = i;
            break;
        }
    }

    // Valor de início de aplicação da mediana para este processo
    int valor_inicial = (cabecalho.altura / DEF_NRO_PROCESSOS * id + 1) - 1;

    // int valor_inicial, int id_processo, int largura, int altura, RGB *valores, RGB *valor_saida, FILE *arq_entrada, FILE *arq_saida
    // aplicar_filtro(init, id, cabecalho.largura, cabecalho.altura, vet, vetSaida, arquivo_entrada, arquivo_saida);
    aplicar_filtro(valor_inicial, id, cabecalho.altura, cabecalho.largura, valores, arquivo_entrada, arquivo_saida, valores_saida);

    // Aguardando os outros processos para finalização
    for (i = 0; i < DEF_NRO_PROCESSOS; i++)
    {
        wait(NULL);
    }

    if (!id)
    {
        salvar_imagem(valor_inicial, id, cabecalho.altura, cabecalho.largura, arquivo_saida, valores_saida);
        shmdt(valores_saida);
        shmctl(shmid, IPC_RMID, NULL);
    }
    else
    {
        shmdt(valores_saida);
        shmctl(shmid, IPC_RMID, NULL);
    }

    free(valores);

    //bug abrir a imagem
    //free(valores_saida);

    fclose(arquivo_entrada);
    fclose(arquivo_saida);
}