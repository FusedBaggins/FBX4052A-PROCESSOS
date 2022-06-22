#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>

#pragma pack(1)
#define DEF_TAMANHO_MASCARA 3
#define DEF_NRO_THREADS 3

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

struct parametros_thread
{
    int id;
    int altura;
    int largura;
    int n_threads;
    int intervalo_inicial;
    int intervalo_final;
    FILE *arquivo_entrada;
    FILE *arquivo_saida;
    RGB **imagem;
    RGB **imagem_modificada;
};
typedef struct parametros_thread PARAMETROS_THREADS;

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

int alinhar(int largura)
{
    return (largura * 3) % 4;
}

void percorrer_imagem(PARAMETROS_THREADS *param_threads)
{
    char c;
    int i, j;
    for (i = 0; i < param_threads->altura; i++)
    {
        // 3 = número bytes do pixel % 4 (pra nao bagunçar as imagens)
        // int alinhamento = alinhar(param_threads->largura);
        // if (alinhamento)
        // {
        //     alinhamento = 4 - alinhamento;
        // }

        for (j = 0; j < param_threads->largura; j++)
        {
            fread(&param_threads->imagem[i][j], sizeof(RGB), 1, param_threads->arquivo_entrada);
        }

        // for (j = 0; j < alinhamento; j++)
        // {
        //     fread(&c, sizeof(unsigned char), 1, param_threads->arquivo_entrada);
        //     fwrite(&c, sizeof(unsigned char), 1, param_threads->arquivo_saida);
        // }
    }
}

void aplicar_filtro(PARAMETROS_THREADS *params)
{
    // Cria os vetores azul, verde e vermelho com valores dependentes da constante MASCARA
    int azul[DEF_TAMANHO_MASCARA * DEF_TAMANHO_MASCARA];
    int verde[DEF_TAMANHO_MASCARA * DEF_TAMANHO_MASCARA];
    int vermelho[DEF_TAMANHO_MASCARA * DEF_TAMANHO_MASCARA];

    int metade_mascara = DEF_TAMANHO_MASCARA / 2;
    int cont_filtro, i = 0, j = 0, ii = 0, jj = 0;
    RGB pixel;

    for (i = params->intervalo_inicial; i < params->intervalo_final; i++)
    {
        for (j = 0; j < params->largura; j++)
        {
            cont_filtro = 0;
            int interromper = 0;

            // Percorre as linhas "geradas" pela máscara
            for (ii = i - metade_mascara; ii < i + metade_mascara; ii++)
            {
                if (interromper)
                {
                    break;
                }

                // Percorre as colunas "geradas" pela máscara
                for (jj = j - metade_mascara; jj <= j + metade_mascara; jj++)
                {
                    if ((ii >= 0) && (ii < params->altura) && (jj >= 0) && (jj <= params->largura))
                    {
                        pixel = params->imagem[ii][jj];
                        azul[cont_filtro] = pixel.blue;
                        verde[cont_filtro] = pixel.green;
                        vermelho[cont_filtro] = pixel.red;
                        cont_filtro++;
                    }
                    else
                    {
                        pixel = params->imagem[i][j];
                        interromper = 1;
                        break;
                    }
                }
            }

            if (!interromper)
            {
                quick_sort(azul, 0, (DEF_TAMANHO_MASCARA * DEF_TAMANHO_MASCARA) - 1);
                quick_sort(verde, 0, (DEF_TAMANHO_MASCARA * DEF_TAMANHO_MASCARA) - 1);
                quick_sort(vermelho, 0, (DEF_TAMANHO_MASCARA * DEF_TAMANHO_MASCARA) - 1);

                pixel.red = vermelho[(DEF_TAMANHO_MASCARA * DEF_TAMANHO_MASCARA) / 2];
                pixel.green = verde[(DEF_TAMANHO_MASCARA * DEF_TAMANHO_MASCARA) / 2];
                pixel.blue = azul[(DEF_TAMANHO_MASCARA * DEF_TAMANHO_MASCARA) / 2];
                params->imagem_modificada[i][j] = pixel;
            }
            else
            {
                params->imagem_modificada[i][j] = pixel;
            }
        }
    }
}

void salvar_imagem(PARAMETROS_THREADS *param_threads)
{
    RGB pixel;
    unsigned char c;

    int i = 0, j = 0;

    for (i = 0; i < param_threads->altura; i++)
    {
        // // 3 = número bytes do pixel % 4 (pra nao bagunçar as imagens)
        // int alinhamento = alinhar(param_threads->largura);
        // if (alinhamento)
        // {
        //     alinhamento = 4 - alinhamento;
        // }
        // Arquivo de saída
        for (j = 0; j < param_threads->largura; j++)
        {
            pixel = param_threads->imagem_modificada[i][j];
            fwrite(&pixel, sizeof(RGB), 1, param_threads->arquivo_saida);
        }

        // for (j = 0; j < alinhamento; j++)
        // {
        //     fread(&c, sizeof(unsigned char), 1, param_threads->arquivo_entrada);
        //     fwrite(&c, sizeof(unsigned char), 1, param_threads->arquivo_saida);
        // }
    }
}

void *executar_thread(void *args)
{
    PARAMETROS_THREADS *param_threads = (PARAMETROS_THREADS *)args;
    aplicar_filtro(param_threads);
}

int main()
{

    FILE *arquivo_saida, *arquivo_entrada;
    CABECALHO cabecalho;

    arquivo_entrada = fopen("borboleta.bmp", "rb");
    arquivo_saida = fopen("p.bmp", "wb");

    // Le o cabecalho inteiro no arquivo de entrada
    fread(&cabecalho, sizeof(CABECALHO), 1, arquivo_entrada);

    // Escreve o cabecalho inteiro no arq de saída

    int i;
    pthread_t *thread_id = NULL;
    PARAMETROS_THREADS *params = NULL;
    params = (PARAMETROS_THREADS *)malloc(DEF_NRO_THREADS * sizeof(PARAMETROS_THREADS));

    thread_id = (pthread_t *)malloc(DEF_NRO_THREADS * sizeof(pthread_t));
    int nroLInhasPorThread = cabecalho.altura / DEF_NRO_THREADS;

    RGB **valores_entrada = (RGB **)malloc(cabecalho.altura * sizeof(RGB *));
    RGB **valores_saida = (RGB **)malloc(cabecalho.altura * sizeof(RGB *));

    for (i = 0; i < cabecalho.altura; i++)
    {
        valores_entrada[i] = (RGB *)malloc(cabecalho.largura * sizeof(RGB));
        valores_saida[i] = (RGB *)malloc(cabecalho.largura * sizeof(RGB));
    }

    for (i = 0; i < DEF_NRO_THREADS; i++)
    {
        params[i].id = i;
        params[i].altura = cabecalho.altura;
        params[i].largura = cabecalho.largura;
        params[i].arquivo_entrada = arquivo_entrada;
        params[i].arquivo_saida = arquivo_saida;
        params[i].imagem = valores_entrada;
        params[i].imagem_modificada = valores_saida;
        params[i].n_threads = DEF_NRO_THREADS;
        params[i].intervalo_inicial = (nroLInhasPorThread * i);
        params[i].intervalo_final = (nroLInhasPorThread * (i + 1));

        if (!i)
        {
            fwrite(&cabecalho, sizeof(CABECALHO), 1, arquivo_saida);
            percorrer_imagem(&params[i]);
        }

        pthread_create(&thread_id[i], NULL, executar_thread, (void *)&params[i]);
    }

    for (i = 0; i < DEF_NRO_THREADS; i++)
    {
        pthread_join(thread_id[i], NULL);
    }

    salvar_imagem(&params[0]);

    for (i = 0; i < cabecalho.altura; i++)
    {
        free(valores_entrada[i]);
        free(valores_saida[i]);
    }

    free(valores_entrada);
    free(valores_saida);

    fclose(arquivo_entrada);
    fclose(arquivo_saida);
}
