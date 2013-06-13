// mpirun -np <QTD_PROCESSOS> ./build/jpeg2raw <QTD_IMAGENS>
// mpirun -np 3 ./build/jpeg2raw 3
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <mpi/mpi.h>

#include <jpeglib.h>

#include <omp.h>

#define MESTRE 0

/* struct to store a grayscale raw image */
struct raw_img {
    uint8_t *img;
    uint8_t num_comp;
    uint32_t width;
    uint32_t height;
};

/* struct to store a grayscale raw image */
struct raw_img2 {
    uint8_t *img;
    size_t width;
    size_t height;
};

#define IMAGE_NCHANNELS 1

//#define QTD_IMAGENS 3

/**
 * save_raw_image - just save a raw grayscale image
 * @param file name of the output file
 * @param raw pointer to a struct raw_img
 */
static void save_raw_image(const char *file, struct raw_img *raw) {
    FILE *raw_file;
    size_t written;
    size_t size;

    raw_file = fopen(file, "w");
    if (raw_file == NULL) {
        printf("Could not open '%s' file\n", file);
        goto error;
    }

    size = raw->width * raw->height;
    written = fwrite((const void *) raw->img, 1, size, raw_file);
    if (written != size)
        printf("Only %lu/%lu bytes were written\n", written, size);

    fclose(raw_file);

error:
    return;
}

/**
 * jpeg_compress - compress a raw grayscale image to a jpeg file
 * @param file name of jpeg file
 * @param raw pointer to struct raw_img
 */
static int jpeg_compress(const char *file, struct raw_img2 *raw) {
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_ptr[1];
    FILE* out_jpeg;
    uint8_t *image;
    int row_stride;
    int err;

    err = 0;
    image = raw->img;
    out_jpeg = fopen(file, "w+");
    if (out_jpeg == NULL) {
        printf("Could no open '%s' file\n", file);
        err = -1;
        goto error;
    }


    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, out_jpeg);

    cinfo.image_width = raw->width;
    cinfo.image_height = raw->height;
    cinfo.input_components = IMAGE_NCHANNELS;
    cinfo.in_color_space = JCS_GRAYSCALE;
    cinfo.err = jpeg_std_error(&jerr);

    jpeg_set_defaults(&cinfo);
    jpeg_start_compress(&cinfo, TRUE);
    row_stride = raw->width * IMAGE_NCHANNELS;

    while (cinfo.next_scanline < cinfo.image_height) {
        row_ptr[0] = &image[cinfo.next_scanline * row_stride];
        jpeg_write_scanlines(&cinfo, row_ptr, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    fclose(out_jpeg);

error:
    return err;
}

int main(int argc, char **argv) {
    int err;
    int myrank, numprocs;
    double inicio, fim;
    FILE * inputFiles[argc - 1];
    char **arquivos[argc - 1];
    char nomeArquivos[30][256];
    char nomeArquivosJPG[30][256];
    char nomeArquivosRAW[30][256];
    char nomeArquivosCINZA[30][256];
    
    if (argc != 2) {
        printf("Número de Parâmetros incorreto. Use: mpirun -np <QTD_PROCESSOS> ./build/jpeg2raw <QTD_IMAGENS>\n");
        err = -1;
        goto error;
    }
    int QTD_IMAGENS = atoi(argv[1]);

    //armazena a quantidade de imagens ainda a serem processadas
    int imagensRestantes = QTD_IMAGENS;
    int imagensProcessadas = 0;
    int controle;
    int acabou = 0; // [0=false,1=true]
    printf("ARGS%d", argc);

    MPI_Status status;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

    inicio = MPI_Wtime();

    int z;
    char str1[40], str2[10], strJPG[40], strRAW[40], strCINZA[40];

    controle = numprocs;

    if ((numprocs - 1) > QTD_IMAGENS) {
        printf("Bah, você está disperdiçando recursos...\n");
        controle = QTD_IMAGENS;
    }

    //Armazena o nome das Imagens a serem processadas
    for (z = 1; z <= QTD_IMAGENS; z++) {
        strcpy(str1, "images/montage");
        sprintf(str2, "%d", z);
        strcat(str1, str2);
        //printf("[str]%s\n", str1);

        strncpy(strJPG, str1, sizeof (str1));
        strncpy(strRAW, str1, sizeof (str1));
        strncpy(strCINZA, str1, sizeof (str1));

        strcpy(nomeArquivos[z], str1);
        strcpy(nomeArquivosJPG[z], strcat(strJPG, ".jpg"));
        strcpy(nomeArquivosRAW[z], strcat(strRAW, ".raw"));
        strcpy(nomeArquivosCINZA[z], strcat(strCINZA, "cinza.jpg"));
    }



    if (numprocs > 1) {
        if (myrank == MESTRE) {
            printf("################################################### \n");
            printf("Número de Processos: %d \n", numprocs);
            printf("Imagens a serem processadas: \n");
            for (z = 1; z <= QTD_IMAGENS; z++) {
                printf("%s\n", nomeArquivos[z]);
            }
            printf("################################################### \n");
            

            //Envia para os primeiros processos
            int tag = 0;
            int x;
            for (x = 1; x < controle; x++) {
                MPI_Send(&x, 1, MPI_INT, x, tag, MPI_COMM_WORLD); //tamanho do array
                imagensRestantes--;
                //printf("Imagens Restantes: %d\n", imagensRestantes);
            }
            int destino;
            int retorno;
            int proxima = controle;
            while (imagensRestantes != 0) {
                MPI_Recv(&retorno, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                imagensProcessadas++;
                destino = status.MPI_SOURCE;
                MPI_Send(&proxima, 1, MPI_INT, status.MPI_SOURCE, tag, MPI_COMM_WORLD); //tamanho do array
                proxima++;
                imagensRestantes--;
            }

            //recebe
            int recebido = 0;
            for (x = 1; x < controle; x++) {
                MPI_Recv(&recebido, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                imagensProcessadas++;
                //envia código para finalizar os escravos (-1)
                int dados = -1;
                MPI_Send(&dados, 1, MPI_INT, status.MPI_SOURCE, tag, MPI_COMM_WORLD); //finalizacao
            }
            fim = MPI_Wtime();
            printf("TEMPO PARALELO[%f] - Imagens Processadas[%d].\n", fim - inicio, imagensProcessadas);
        } else {
            while (acabou != 1) {
                int posicao;
                // int *arrayReceiving;
                MPI_Recv(&posicao, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

                if (posicao == -1) {
                    acabou = 1;
                } else {
                    printf("\n[%d]Processando a Imagem %s\n", myrank, nomeArquivosJPG[posicao]);
                    struct raw_img raw_img;
                    err = 0;
                    FILE *jpeg_file;
                    struct jpeg_decompress_struct jdec;
                    struct jpeg_error_mgr jerr;
                    JSAMPROW row_pointer[1];
                    float r;
                    float g;
                    float b;
                    int i;
                    int j;
                    j = 0;

                    /**
                     * abre um arquivo JPEG no espaço de cores RGB e extrai a luminância  
                     * (imagem em tons de cinza), que é gravada em um arquivo especificado pelo usuário.
                     */
                    const char *file = argv[1];
                    struct raw_img *raw = &raw_img;

                    //err = jpeg_decompress(argv[1], &raw_img, myrank, numprocs);

                    //carrega o arquivo de imagem jpeg
                    jpeg_file = fopen(nomeArquivosJPG[posicao], "rb");
                    if (jpeg_file == NULL) {
                        printf("Error opening file %s\n!", nomeArquivosJPG[posicao]);
                        err = -1;
                        goto error;
                    }

                    jdec.err = jpeg_std_error(&jerr);
                    jpeg_create_decompress(&jdec);
                    jpeg_stdio_src(&jdec, jpeg_file);
                    jpeg_read_header(&jdec, TRUE);

                    raw->width = jdec.image_width;
                    raw->height = jdec.image_height;
                    raw->num_comp = jdec.num_components;

                    if (raw->num_comp != 3) {
                        printf("Cannot convert a jpeg file with %d componentes per "
                                "pixel\n", raw->num_comp);
                        err = -1;
                        goto error;
                    }

                    //INFORMAÇÕES DA IMAGEM
                    printf("JPEG File Information:\n");
                    printf("Resolução: %d x %d pixels\n", raw->width, raw->height);
                    //printf("\tColor components per pixel: %d\n", raw->num_comp);

                    jpeg_start_decompress(&jdec);
                    raw->img = (uint8_t *) malloc(raw->width * raw->height);
                    row_pointer[0] = (unsigned char *) malloc(raw->width * raw->num_comp);

                    while (jdec.output_scanline < jdec.image_height) {
                        jpeg_read_scanlines(&jdec, row_pointer, 1);
                        for (i = 0; i < raw->width * raw->num_comp; i += 3) {
                            r = (float) row_pointer[0][i];
                            g = (float) row_pointer[0][i + 1];
                            b = (float) row_pointer[0][i + 2];
                            /* here convert to grayscale */
                            raw->img[j] = (uint8_t)
                                    (0.2126 * r + 0.7152 * g + 0.0722 * b);
                            j++;
                        }
                    }

                    jpeg_finish_decompress(&jdec);
                    jpeg_destroy_decompress(&jdec);
                    free(row_pointer[0]);
                    fclose(jpeg_file);

                    if (err == -1)
                        goto error;
                    //	save_raw_image(argv[2], &raw_img);
                    save_raw_image(nomeArquivosRAW[posicao], &raw_img);
                    free(raw_img.img);

                    //Convere o RAW para imagem em tons de cinza
                    //SERIA o SEGUNDO COMANDO QUE É EXECUTADO
                    struct raw_img2 raw_img2;
                    err = 0;
                    raw_img2.width = raw->width;
                    raw_img2.height = raw->height;
                    // printf("DEBUG2 widht[%d] height[%d]\n", raw_img2.width, raw_img2.height);

                    // err = open_raw_image(argv[1], &raw_img);
                    //open_raw_image(const char *file, struct raw_img *raw)
                    const char *file2 = nomeArquivosRAW[posicao];
                    struct raw_img2 *raw2 = &raw_img2;
                    FILE *raw_file;
                    size_t read;
                    size_t size;

                    //	raw_file = fopen(file2, "r");
                    raw_file = fopen(file2, "r");
                    if (raw_file == NULL) {
                        printf("Could not open '%s' file\n", file2);
                        err = -1;
                        goto error;
                    }
                    // printf("Abriu arquivo RAW\n");
                    size = raw2->width * raw2->height;
                    raw2->img = (uint8_t *) malloc(size * sizeof (uint8_t));
                    if (raw2->img == NULL) {
                        err = -2;
                        goto error;
                    }
                    read = fread((void *) raw2->img, 1, size, raw_file);
                    if (read != size)
                        printf("Only %lu/%lu bytes were read\n", read, size);

                    fclose(raw_file);

                    if (err == -1)
                        goto error;
                    if (err == -2)
                        goto error;
                    jpeg_compress(nomeArquivosCINZA[posicao], &raw_img2);
                    MPI_Send(&err, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                }
            }
        }
    } else { //FAZ SEQUENCIAL
        for (z = 1; z <= QTD_IMAGENS; z++) {
            int posicao = z;
            struct raw_img raw_img;

            err = 0;


            FILE *jpeg_file;
            struct jpeg_decompress_struct jdec;
            struct jpeg_error_mgr jerr;
            JSAMPROW row_pointer[1];
            float r;
            float g;
            float b;
            int i;
            int j;
            err = 0;
            j = 0;




            /**
             * abre um arquivo JPEG no espaço de cores RGB e extrai a luminância  
             * (imagem em tons de cinza), que é gravada em um arquivo especificado pelo usuário.
             */
            const char *file = argv[1];
            struct raw_img *raw = &raw_img;

            //err = jpeg_decompress(argv[1], &raw_img, myrank, numprocs);

            //carrega o arquivo de imagem jpeg
            jpeg_file = fopen(nomeArquivosJPG[posicao], "rb");
            //	jpeg_file = fopen(arquivos[1], "rb");
            //        printf("%p\n",jpeg_file);
            //        printf("%s\n",jpeg_file);
            if (jpeg_file == NULL) {
                printf("Error opening file %s\n!", nomeArquivosJPG[posicao]);
                err = -1;
                goto error;
            }

            jdec.err = jpeg_std_error(&jerr);
            jpeg_create_decompress(&jdec);
            jpeg_stdio_src(&jdec, jpeg_file);
            jpeg_read_header(&jdec, TRUE);


            raw->width = jdec.image_width;
            raw->height = jdec.image_height;
            raw->num_comp = jdec.num_components;

            printf("[DEBUG] WIDTH[%d] HEIGHT[%d] COMP[%d]\n", jdec.image_width, jdec.image_height, jdec.num_components);
            printf("[DEBUG] WIDTH[%d] HEIGHT[%d] COMP[%d]\n", raw->width, raw->height, raw->num_comp);

            if (raw->num_comp != 3) {
                printf("Cannot convert a jpeg file with %d componentes per "
                        "pixel\n", raw->num_comp);
                err = -1;
                goto error;
            }

            //        int altura = raw->height;
            //        int partes = raw->height/(numprocs-1); //desconta 1 por causa do mestre
            //        int resto = raw->height%(numprocs-1);  //desconta 1 por causa do mestre
            //        printf("Altura %u \n", altura);
            if (myrank == MESTRE) {
                //            printf("raw->height %.f \n",raw->height/numprocs);
                //            printf("Altura %u \n", altura);
                //            printf("Processos %d Divisão %d \n", numprocs, partes);
                //            printf("Resto %u \n", resto);
                //INFORMAÇÕES DA IMAGEM
                printf("JPEG File Information:\n");
                printf("\tResolution: %d x %d pixels\n", raw->width, raw->height);
                printf("\tColor components per pixel: %d\n", raw->num_comp);
            } else {
                //            printf("Sou o processo[%d] e vou calculdar de [%d - %d]\n", myrank, ((myrank-1)*partes), (myrank*partes));
            }

            jpeg_start_decompress(&jdec);

            raw->img = (uint8_t *) malloc(raw->width * raw->height);
            row_pointer[0] = (unsigned char *) malloc(raw->width * raw->num_comp);

            printf("\t raw->width * raw->num_comp: %d\n", raw->width * raw->num_comp);


            while (jdec.output_scanline < jdec.image_height) {
                jpeg_read_scanlines(&jdec, row_pointer, 1);
                //                #pragma omp parallel for
                //                #pragma omp for schedule(static) ordered
                //                 #pragma omp parallel
                // {
                //                 #pragma omp for schedule(static) ordered
                //                 #pragma omp for ordered private(i,j)
                for (i = 0; i < raw->width * raw->num_comp; i += 3) {
                    r = (float) row_pointer[0][i];
                    g = (float) row_pointer[0][i + 1];
                    b = (float) row_pointer[0][i + 2];
                    /* here convert to grayscale */
                    //                        #pragma omp ordered 
                    //                        #pragma omp critical
                    //                        {
                    raw->img[j] = (uint8_t)
                            (0.2126 * r + 0.7152 * g + 0.0722 * b);
                    j++;
                    //                        }
                    //		}
                }
            }
            printf("J %d \n", j);

            jpeg_finish_decompress(&jdec);
            jpeg_destroy_decompress(&jdec);
            free(row_pointer[0]);
            fclose(jpeg_file);

            if (err == -1)
                goto error;
            //	save_raw_image(argv[2], &raw_img);
            save_raw_image(nomeArquivosRAW[posicao], &raw_img);
            free(raw_img.img);


            //Convere o RAW para imagem em tons de cinza
            //SERIA o SEGUNDO COMANDO QUE É EXECUTADO
            struct raw_img2 raw_img2;
            err = 0;
            //	if (argc != 5) {
            //		printf("Usage: %s <raw_file> <jpeg> <width> <height>\n",argv[0]);
            //		err = -1;
            //		goto error;
            //	}

            raw_img2.width = raw->width;
            raw_img2.height = raw->height;
            printf("DEBUG2 widht[%d] height[%d]\n", raw_img2.width, raw_img2.height);

            // err = open_raw_image(argv[1], &raw_img);
            //open_raw_image(const char *file, struct raw_img *raw)
            const char *file2 = nomeArquivosRAW[posicao];
            struct raw_img2 *raw2 = &raw_img2;
            FILE *raw_file;
            size_t read;
            size_t size;

            //	raw_file = fopen(file2, "r");
            raw_file = fopen(file2, "r");
            if (raw_file == NULL) {
                printf("Could not open '%s' file\n", file2);
                err = -1;
                goto error;
            }
            printf("Abriu arquivo RAW\n");
            size = raw2->width * raw2->height;
            raw2->img = (uint8_t *) malloc(size * sizeof (uint8_t));
            if (raw2->img == NULL) {
                err = -2;
                goto error;
            }
            read = fread((void *) raw2->img, 1, size, raw_file);
            if (read != size)
                printf("Only %lu/%lu bytes were read\n", read, size);

            fclose(raw_file);

            if (err == -1)
                goto error;
            if (err == -2)
                goto error;
            jpeg_compress(nomeArquivosCINZA[posicao], &raw_img2);
        }
        fim = MPI_Wtime();
        printf("TEMPO SEQ[%f].\n", fim - inicio);
    }
    // fim = MPI_Wtime();

    MPI_Finalize();
    return 0;
    //cleanup:
    //	free(raw_img2.img);
error:
    return err;
}