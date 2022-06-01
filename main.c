#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#pragma pack(1)

#define MASCARA 7
#define NRO_PROCESSOS 4

struct cabecalho
{
    unsigned short tipo;
    unsigned int tamanho_arquivo;
    unsigned short reservado1;
    unsigned short reservado2;
    unsigned int offset;
    unsigned int tamanho_image_header;
    int largura;
    int altura;
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

void quick_sort(int *a, int left, int right)
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
        quick_sort(a, left, j);
    }
    if (i < right)
    {
        quick_sort(a, i, right);
    }
}

void bubble_sort(int *vetor, int n)
{
    int k, j, aux;

    for (k = 1; k < n; k++)
    {
        for (j = 0; j < n - 1; j++)
        {
            if (vetor[j] > vetor[j + 1])
            {
                aux = vetor[j];
                vetor[j] = vetor[j + 1];
                vetor[j + 1] = aux;
            }
        }
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
        int alinhamento = alinhar(largura);
        if (alinhamento)
        {
            alinhamento = 4 - alinhamento;
        }

        for (j = 0; j < largura; j++)
        {
            fread(&valores[i * largura + j], sizeof(RGB), 1, arq_entrada);
        }

        for (j = 0; j < alinhamento; j++)
        {
            fread(&c, sizeof(unsigned char), 1, arq_entrada);
            fwrite(&c, sizeof(unsigned char), 1, arq_saida);
        }
    }
}

void aplicar_filtro(int valor_inicial, int id_processo, int altura, int largura, RGB *valores, FILE *arq_entrada, FILE *arq_saida, RGB *valores_saida)
{

    // Cria os vetores azul, verde e vermelho com valores dependentes da constante MASCARA
    int azul[MASCARA * MASCARA];
    int verde[MASCARA * MASCARA];
    int vermelho[MASCARA * MASCARA];

    int metade_mascara = MASCARA / 2;
    int cont_filtro, i = 0, j = 0, ii = 0, jj = 0;
    RGB pixel;

    for (i = valor_inicial; i < altura / NRO_PROCESSOS * (id_processo + 1); i++)
    {
        for (j = 0; j < largura; j++)
        {
            cont_filtro = 0;
            int interromper = 0;

            // Percorre as linhas "geradas" pela máscara
            for (ii = i - metade_mascara; ii <= i + metade_mascara; ii++)
            {
                if (interromper)
                {
                    break;
                }

                // Percorre as colunas "geradas" pela máscara
                for (jj = j - metade_mascara; jj <= j + metade_mascara; jj++)
                {
                    if ((ii >= 0) && (ii <= altura) && (jj >= 0) && (jj <= largura))
                    {
                        pixel = valores[ii * largura + jj];
                        azul[cont_filtro] = pixel.blue;
                        verde[cont_filtro] = pixel.green;
                        vermelho[cont_filtro] = pixel.red;
                        cont_filtro++;
                    }
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
                int tamanho = (MASCARA * MASCARA) - 1;
                quick_sort(azul, 0, tamanho);
                quick_sort(verde, 0, tamanho);
                quick_sort(vermelho, 0, tamanho);
                // bubble_sort(azul, tamanho);
                // bubble_sort(vermelho, tamanho);
                // bubble_sort(verde, tamanho);
                pixel.red = vermelho[(MASCARA * MASCARA) / 2];
                pixel.green = verde[(MASCARA * MASCARA) / 2];
                pixel.blue = azul[(MASCARA * MASCARA) / 2];
                valores_saida[i * largura + j] = pixel;
            }
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
        int alinhamento = alinhar(largura);

        if (alinhamento)
        {
            alinhamento = 4 - alinhamento;
        }

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

int main(int argc, char **argv)
{
    int shmid;
    CABECALHO cabecalho;
    FILE *arq_entrada, *arq_saida;
    char nome_arquivo_entrada[100], nome_arquivo_saida[100];

    printf("Digite o nome do arquivo de entrada:\n");
    scanf("%s", nome_arquivo_entrada);

    printf("Digite o nome do arquivo de saida:\n");
    scanf("%s", nome_arquivo_saida);

    arq_entrada = fopen(nome_arquivo_entrada, "rb");

    if (arq_entrada == NULL)
    {
        printf("Erro ao abrir o arquivo %s\n", nome_arquivo_entrada);
        exit(0);
    }

    arq_saida = fopen(nome_arquivo_saida, "wb");

    if (arq_saida == NULL)
    {
        printf("Erro ao abrir o arquivo %s\n", nome_arquivo_saida);
        exit(0);
    }

    fread(&cabecalho, sizeof(CABECALHO), 1, arq_entrada);
    fwrite(&cabecalho, sizeof(CABECALHO), 1, arq_saida);

    shmid = shmget(5, cabecalho.altura * cabecalho.largura * sizeof(RGB), IPC_CREAT | 0600);

    RGB *valores_saida = shmat(shmid, 0, 0);
    RGB *valores = (RGB *)malloc(cabecalho.altura * cabecalho.largura * sizeof(RGB));

    percorrer_imagem(cabecalho.altura, cabecalho.largura, valores, arq_entrada, arq_saida);

    int pid, id = 0, i;
    for (i = 1; i < NRO_PROCESSOS; i++)
    {
        pid = fork();
        if (pid == 0)
        {
            id = i;
            break;
        }
    }

    int valor_inicial = (cabecalho.altura / NRO_PROCESSOS * id + 1) - 1;
    aplicar_filtro(valor_inicial, id, cabecalho.altura, cabecalho.largura, valores, arq_entrada, arq_saida, valores_saida);

    for (i = 0; i < NRO_PROCESSOS; i++)
    {
        wait(NULL);
    }

    if (!id)
    {
        salvar_imagem(valor_inicial, id, cabecalho.altura, cabecalho.largura, arq_saida, valores_saida);
        shmdt(valores_saida);
        shmctl(shmid, IPC_RMID, NULL);
    }
    else
    {
        shmdt(valores_saida);
        shmctl(shmid, IPC_RMID, NULL);
    }

    free(valores);

    // bug abrir a imagem
    // free(valores_saida);

    fclose(arq_entrada);
    fclose(arq_saida);
}