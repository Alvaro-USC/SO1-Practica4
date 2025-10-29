#define _GNU_SOURCE 
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>    // Para clock_gettime
#include <sched.h>   // Para sched_yield



#define TOTAL_POINTS 1000000000
#define YIELD_FREQUENCY 1000000
#define TOTAL_HILOS 12


// Estructura para pasar argumentos a cada hilo
typedef struct {
    int id;
    int use_yield; // 1 = Generoso (usa yield), 0 = Competitivo (no usa)
    unsigned int seed; // Semilla para rand_r (thread-safe)
} ThreadArgs;


// Función que ejecutará cada hilo
void* calcular_pi(void* arg) {
    ThreadArgs* args = (ThreadArgs*) arg;
    const char* tipo = args->use_yield ? "GENEROSO" : "COMPETITIVO";

    // 1. Preparar medición de tiempo
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    printf("Hilo %d de tipo %s: \tIniciado. Calculando %d puntos.\n", 
           args->id, tipo, TOTAL_POINTS);

    unsigned long hits = 0;
    // Hacemos una copia local de la semilla para rand_r
    unsigned int seed = args->seed; 

    // Bucle de cálculo intensivo (Método Montecarlo)
    for (long i = 0; i < TOTAL_POINTS; i++) {
        
        // Generar puntos aleatorios (x, y) entre 0 y 1
        // Usamos rand_r por ser thread-safe
        double x = (double)rand_r(&seed) / RAND_MAX;
        double y = (double)rand_r(&seed) / RAND_MAX;

        // Comprobar si (x^2 + y^2 <= 1)
        if (x*x + y*y <= 1.0) {
            hits++;
        }

        // Si el hilo es "Generoso", cede la CPU 
        if (args->use_yield && (i % YIELD_FREQUENCY) == 0) {
            sched_yield(); 
        }
    }

    // Calcular resultado y tiempo
    double pi_estimate = 4.0 * (double)hits / (double)TOTAL_POINTS;
    
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    
    // Calcular tiempo total de ejecución del hilo
    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) + 
                          (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    printf("--- [Hilo %d (%s)]: \tTERMINADO. (Pi ≈ %f) (Tiempo: %.2f seg) ---\n", 
           args->id, tipo, pi_estimate, elapsed_time);

    // Devolvemos el resultado (opcional, pero buena práctica)
    double* result = malloc(sizeof(double));
    *result = pi_estimate;
    pthread_exit(result);
}


// --- Función Principal ---
int main() {
    pthread_t hilos[TOTAL_HILOS];
    ThreadArgs args[TOTAL_HILOS];
    
    printf("Iniciando %d hilos (%d Competitivos, %d Generosos).\n", 
           TOTAL_HILOS, TOTAL_HILOS/2, TOTAL_HILOS/2);
    printf("Cada hilo calculará %d puntos.\n", TOTAL_POINTS);
    printf("ADVERTENCIA: 1e9 puntos es un cálculo largo, puede tardar varios minutos.\n\n");

    int hilo_count = 0;

    // Crear hilos COMPETITIVOS
    for (int i = 1; i < TOTAL_HILOS; ++i, ++hilo_count) {
        args[hilo_count].id = hilo_count;
        if (i % 2) args[hilo_count].use_yield = 1; // 1 = Generoso
        else args[hilo_count].use_yield = 0;
        args[hilo_count].seed = time(NULL) + hilo_count;
        pthread_create(&hilos[hilo_count], NULL, calcular_pi, &args[hilo_count]);
    }


    // Esperar a que todos los hilos terminen
    double pi_total = 0.0;
    for (int i = 0; i < TOTAL_HILOS; i++) {
        void* result_ptr;
        pthread_join(hilos[i], &result_ptr);
        
        double* pi_result = (double*) result_ptr;
        pi_total += *pi_result;
        free(pi_result); // Liberar la memoria que asignamos en el hilo
    }

    printf("\nTodos los hilos han terminado.\n");
    printf("Promedio de Pi calculado: %f\n", pi_total / TOTAL_HILOS);
    
    return 0;
}