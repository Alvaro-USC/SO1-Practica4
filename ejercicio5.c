#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>    // Para clock_gettime
#include <sched.h>   // Para sched_yield



#define TOTAL_POINTS 1000000000
#define YIELD_FREQUENCY 1000000
#define TOTAL_HILOS 10


// Estructura para pasar argumentos a cada hilo
typedef struct {
    int id;
    int use_yield; // 1 = Generoso (usa yield), 0 = Competitivo (no usa)
    unsigned int seed; // Semilla para rand_r 
    double thread_time;
    double pi_approx;
} ThreadArgs;

// Función que ejecutará cada hilo
void* calcular_pi(void* arg) {
    ThreadArgs* args = (ThreadArgs*) arg;

    // Preparar medición de tiempo
    struct timespec start_time, end_time;
    // CLOCK_MONOTONIC hace que el tiempo que está en yield se contabilice
    clock_gettime(CLOCK_MONOTONIC, &start_time);  

    unsigned long hits = 0;
    
    // Bucle de cálculo intensivo (Método Montecarlo)
    for (long i = 0; i < TOTAL_POINTS; i++) {
        
        // Generar puntos aleatorios (x, y) entre 0 y 1
        // Usamos rand_r por ser thread-safe y evitar que se corrompa por acceso al mismo tiempo
        double x = (double)rand_r(&args->seed) / RAND_MAX;
        double y = (double)rand_r(&args->seed) / RAND_MAX;

        // Comprobar si (x^2 + y^2 <= 1)
        if (x*x + y*y <= 1.0) {
            hits++;
        }

        // Si el hilo es "Generoso", cede la CPU tras calcular YIELD_FREQUENCY puntos cada vez
        if (args->use_yield && (i % YIELD_FREQUENCY) == 0) {
            sched_yield();  // Cambio de contexto del hilo generoso a otro hilo competitivo
        }
    }

    // Calcular resultado y tiempo
    double pi_estimate = 4.0 * (double)hits / (double)TOTAL_POINTS;
    
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    
    // Calcular tiempo total de ejecución del hilo
    double elapsed_time = (end_time.tv_sec - start_time.tv_sec) + 
                          (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    args->thread_time = elapsed_time;
    args->pi_approx = pi_estimate;
    pthread_exit(0);
}


int comparar_resultados(const void* hilo1, const void* hilo2) {
    ThreadArgs* res_a = (ThreadArgs*) hilo1;
    ThreadArgs* res_b = (ThreadArgs*) hilo2;
    if ((res_a)->thread_time < (res_b)->thread_time) return -1;
    if ((res_a)->thread_time > (res_b)->thread_time) return 1;
    return 0;
}

// Función Principal
int main() {
    pthread_t hilos[TOTAL_HILOS];
    ThreadArgs args[TOTAL_HILOS];
    
    printf("Iniciando %d hilos (%d Competitivos, %d Generosos).\n", 
           TOTAL_HILOS, TOTAL_HILOS/2, TOTAL_HILOS/2);
    printf("Cada hilo calculará %d puntos.\n", TOTAL_POINTS);

    int hilo_count = 0;

    // Crear hilos COMPETITIVOS
    for (int i = 0; i < TOTAL_HILOS; ++i, ++hilo_count) {
        args[hilo_count].id = hilo_count;
        if (i % 2 == 0) 
            args[hilo_count].use_yield = 1; // 1 = Generoso
        else 
            args[hilo_count].use_yield = 0;
        args[hilo_count].seed = time(NULL) + hilo_count;
        pthread_create(&hilos[hilo_count], NULL, calcular_pi, &args[hilo_count]);
    }


    // Esperar a que todos los hilos terminen
    double pi_total = 0.0;
    for (int i = 0; i < TOTAL_HILOS; i++)
        pthread_join(hilos[i], NULL);

    
    qsort(args, TOTAL_HILOS, sizeof(ThreadArgs), comparar_resultados);
    printf("ID HILO\t\tTIPO\t\tTIEMPO\t\tPI APPROX\n");
    for (int i = 0; i < TOTAL_HILOS; ++i)
    {
        char* string;
        if (args[i].use_yield % 2) string = "GENEROSO";
        else string = "COMPETITIVO";
        pi_total += args[i].pi_approx;
         
        printf("%d\t\t%s\t%lf\t%lf\n", args[i].id, string, args[i].thread_time, args[i].pi_approx);
    }

    // (sizeof(args) / sizeof(args[0])) - 1 es igual a TOTAL_HILOS - 1, es para coger el tiempo del útlimo hilo
    // que por la ordenación del qsort, será el tiempo total
    printf("\nTodos los hilos han terminado, tardaron %lf.\n", args[(sizeof(args) / sizeof(args[0])) - 1].thread_time);
    printf("Promedio de Pi calculado: %f\n", pi_total / TOTAL_HILOS);
    
    return 0;
}