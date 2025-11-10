#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>       // pow() y M_PI
#include <time.h>       // clock_gettime

#define N_ITERATIONS 1000000000
#define HILOS 10
#define MODO_COMPROBACION 0   

// Estructura para pasar argumentos a cada hilo
typedef struct {
    int id;
} ThreadArgs;

double obtener_termino(long long n) {
#if MODO_COMPROBACION == 1
    return 1.0;
#else
    return 1.0 / pow((double)n, 2.0);
#endif
}

void* hilo_calculo_basilea(void* arg) {
    ThreadArgs* args = (ThreadArgs*) arg;
    
    // Reparto estático por bloques
    long long tam_bloque = N_ITERATIONS / HILOS;
    long long restante = N_ITERATIONS % HILOS;

    long long start, end;

    if (args->id == 0) {
        // Iteraciones sobrantes (restante) se asignan al primer hilo
        start = 1;
        end = tam_bloque + restante;
    } else {
        // Comienzo de los demás hilos (contando desde 1)
        start = (tam_bloque * args->id) + restante + 1;
        end = (tam_bloque * (args->id + 1)) + restante;
    }

    // Cada hilo calcula sobre su propia variable sum_parcial para evitar condiciones de carrera
    // y error de multiple acceso, debería ser atómico
    double sum_parcial = 0.0;
    for (long long n = start; n <= end; n++) {
        sum_parcial += obtener_termino(n);
    }

    // Devolvemos el resultado parcial (requiere malloc para que no esté en el stack 
    // y no se libere al terminar ejecución)
    double* resultado = malloc(sizeof(double));
    *resultado = sum_parcial;
    pthread_exit(resultado);
}

// Cálculo completo de forma secuencial.
double calculo_secuencial() {
    double sum = 0.0;
    for (long long n = 1; n <= N_ITERATIONS; n++) {
        sum += obtener_termino(n);
    }
    return sum;
}


int main() {
    pthread_t hilos[HILOS];
    ThreadArgs args[HILOS];
    
    struct timespec start, end;
    double tiempo_paralelo, tiempo_secuencial;
    
    printf("Calculando Problema de Basilea para N = %d\n", N_ITERATIONS);
    printf("Empezamos con el cálculo en paralelo usando %d hilos.\n", HILOS);

    // CÁLCULO PARALELO (hilo a hilo)
    double sum_total_paralelo = 0.0;
    // MONOTONIC es para contabilizar posibles yields (en este caso no hay)
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Crear hilos
    for (int i = 0; i < HILOS; i++) {
        args[i].id = i;
        pthread_create(&hilos[i], NULL, hilo_calculo_basilea, &args[i]);
    }

    // Esperar hilos y sumar sus resultados calculados (join)
    for (int i = 0; i < HILOS; i++) {
        void* resultado_parcial;
        pthread_join(hilos[i], &resultado_parcial);
        
        sum_total_paralelo += *(double*)resultado_parcial;
        
        free(resultado_parcial); // Liberar la memoria del hilo por el malloc
    }
    
    // Final de ejecución paralelo
    clock_gettime(CLOCK_MONOTONIC, &end);
    tiempo_paralelo = (end.tv_sec - start.tv_sec) + 
                      (end.tv_nsec - start.tv_nsec) / 1e9;

    // CÁLCULO SECUENCIAL 
    
    printf("Calculando versión secuencial (un solo hilo)...\n");
    clock_gettime(CLOCK_MONOTONIC, &start);
    double sum_total_secuencial = calculo_secuencial();
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    tiempo_secuencial = (end.tv_sec - start.tv_sec) + 
                        (end.tv_nsec - start.tv_nsec) / 1e9;
    
    double valor_exacto = (M_PI * M_PI) / 6.0; // basilea exactamente debe ser pi cuadrado partido por 6

    printf("\nResultados\n");
    printf("Valor Paralelo:   %.15f\n", sum_total_paralelo);
    printf("Valor Secuencial: %.15f\n", sum_total_secuencial);
    printf("Son iguales: %d\n", sum_total_paralelo == sum_total_secuencial);
    printf("Valor Exacto (PI^2 / 6): %.15f\n", valor_exacto);
    printf("Paralelo vs Secuencial: %e\n", sum_total_paralelo - sum_total_secuencial);
    printf("Paralelo vs Exacto:     %e\n", sum_total_paralelo - valor_exacto);

    printf("\nTiempos de Ejecución\n");
    printf("Tiempo Paralelo (%d hilos): %f segundos\n", HILOS, tiempo_paralelo);
    printf("Tiempo Secuencial (1 hilo): %f segundos\n", tiempo_secuencial);

    return 0;
}