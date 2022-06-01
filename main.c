#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

int alinhar(int largura)
{
    return (largura * 3) % 4;
}

void aplicar_filtro() {}

void abrir_imagem(int largura, int altura, FILE *arq_entrada, FILE *arq_saida, RGB *valores)
{
    char c;
    int i = 0, j = 0;

    for (i = 0; i < altura; i++)
    {
        int alinhamento = alinhar(largura);
        if (alinhamento)
            alinhamento = 4 - alinhamento;

        for (j = 0; j < largura; j++)
            fread(&valores[i * largura * j], sizeof(RGB), 1, arq_entrada);

        for (j = 0; j < alinhamento; j++)
        {
            fread(&c, sizeof(unsigned char), 1, arq_entrada);
            fwrite(&c, sizeof(unsigned char), 1, arq_saida);
        }
    }
}

void salvar_imagem()
{
}

int main(int argc)
{
}