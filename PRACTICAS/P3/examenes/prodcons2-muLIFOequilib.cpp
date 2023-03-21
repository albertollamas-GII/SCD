// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: prodcons2.cpp
// Implementación del problema del productor-consumidor con
// un proceso intermedio que gestiona un buffer finito y recibe peticiones
// en orden arbitrario
// (versión con un único productor y un único consumidor)
//
// Historial:
// Actualizado a C++11 en Septiembre de 2017
// -----------------------------------------------------------------------------
//**********************************************************************
// Compilación y ejecución
// mpicxx -std=c++11 -o prodcons2-muLIFOequi prodcons2-muLIFOequilib.cpp
// mpirun --oversubscribe -np 11 ./prodcons2-muLIFOequi
//**********************************************************************

#include <iostream>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <mpi.h>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

const int
   
   tam_vector            = 10,

   num_prod              = 6,
   num_cons              = 4 ,
   id_buffer             = 10 ,

   etiq_prod             = 1,
   etiq_cons             = 2,
   num_procesos_esperado = num_prod + num_cons + 1, // (+1 --> buffer)
   num_items             = 24; //cada productor producirá 15 elementos (15 * 6 = 90)

   int k_prod = num_items / num_prod; //num items a producir por cada productora
   int k_cons = num_items / num_cons; //num items a producir por cada productora
   unsigned int cont_prod_hebra[num_prod] = {0}; //contador que indica nº de items producidos por cada hebra

   unsigned int cont_cons_hebra[num_cons] = {0}; //contador que indica nº de items consumidos por cada hebra

//**********************************************************************
// plantilla de función para generar un entero aleatorio uniformemente
// distribuido entre dos valores enteros, ambos incluidos
// (ambos tienen que ser dos constantes, conocidas en tiempo de compilación)
//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}
// ---------------------------------------------------------------------
// ptoducir produce los numeros en secuencia (1,2,3,....)
// y lleva espera aleatorio
int producir( int num_prod)
{
   //static int contador = 0 ;
   sleep_for( milliseconds( aleatorio<10,100>()) );
   //contador++ ;
   int valor_producido = num_prod * k_prod + cont_prod_hebra[num_prod];
   cont_prod_hebra[num_prod]++;
   cout << "(p" << num_prod << ") Productor ha PRODUCIDO valor " << valor_producido << endl << flush;
   //return contador ;
   return valor_producido;
}
// ---------------------------------------------------------------------
          
void funcion_productor( int num_prod)
{  
   for ( unsigned int i= 0 ; i < k_prod ; i++ )
   {
      // producir valor
      int valor_prod = producir( num_prod);
      // enviar valor
      cout << " --> (p" << num_prod << ") Productor va a enviar valor " << valor_prod << endl << flush;
      MPI_Ssend( &valor_prod, 1, MPI_INT, id_buffer, etiq_prod, MPI_COMM_WORLD );
   }
}
// ---------------------------------------------------------------------

void consumir( int valor_cons,  int num_cons )
{
   // espera bloqueada
   cont_cons_hebra[num_cons]++; 
   sleep_for( milliseconds( aleatorio<110,200>()) );
   cout << "(c" << num_cons << ") Consumidor ha CONSUMIDO valor " << valor_cons << endl << flush ;
}
// ---------------------------------------------------------------------

void funcion_consumidor( int num_cons)
{
   int         peticion,
               valor_rec = 1 ;
   MPI_Status  estado ;

   for( unsigned int i=0 ; i < k_cons; i++ )
   {
      MPI_Ssend( &peticion,  1, MPI_INT, id_buffer, etiq_cons, MPI_COMM_WORLD);
      //MPI_Recv ( &valor_rec, 1, MPI_INT, id_buffer, etiq_cons, MPI_COMM_WORLD,&estado );
      MPI_Recv ( &valor_rec, 1, MPI_INT, id_buffer, etiq_cons, MPI_COMM_WORLD,&estado );
      cout << " <-- (c" << num_cons << ") Consumidor ha recibido valor " << valor_rec << endl << flush ;
      consumir( valor_rec, num_cons);
   }
}
// ---------------------------------------------------------------------

void funcion_buffer()
{
   int        buffer[tam_vector],      // buffer con celdas ocupadas y vacías
              valor,                   // valor recibido o enviado
              primera_libre       = 0, // índice de primera celda libre
              num_celdas_ocupadas = 0, // número de celdas ocupadas
              etiq_emisor_aceptable;    // etiqueta de emisor aceptable
   MPI_Status estado ;                 // metadatos del mensaje recibido

   int prod_seguidos = 0;

   for( unsigned int i=0 ; i < num_items*2 ; i++ )
   {
      // 1. determinar si puede enviar solo prod., solo cons, o todos
      if ( num_celdas_ocupadas == 0 ) {// si buffer vacío
         etiq_emisor_aceptable = etiq_prod ;          // $~~~$ solo prod.
      }  
      else if ( num_celdas_ocupadas == tam_vector || prod_seguidos == 4){// si buffer lleno 
                                                          // o ya ha atendido a 4 productores seguidos
         //cout << "******** BUFFER LLENO ********" << endl ;
         if (prod_seguidos == 4)
            cout << "**** Ya he atendido 4 prods seguidos, ahora toca CONSUMIR ****" << endl ;
         etiq_emisor_aceptable = etiq_cons ;          // $~~~$ solo cons.
      } 
      else{                                           // si no vacío ni lleno    
         etiq_emisor_aceptable = MPI_ANY_TAG;     // $~~~$ cualquiera
      }   

      // 2. recibir un mensaje del emisor o emisores aceptables
      MPI_Recv( &valor, 1, MPI_INT, MPI_ANY_SOURCE, etiq_emisor_aceptable, MPI_COMM_WORLD, &estado );

      // 3. procesar el mensaje recibido
      switch( estado.MPI_TAG ) // leer emisor del mensaje en metadatos
      {
         case etiq_prod: // si ha sido el productor: insertar en buffer
            buffer[primera_libre] = valor ;
            primera_libre++;
            num_celdas_ocupadas++ ;
            cout << "Buffer ha recibido valor " << valor << endl ;
            prod_seguidos++; //atendemos a un productor más
            break;

         case etiq_cons: // si ha sido el consumidor: extraer y enviarle
            primera_libre--;
            valor = buffer[primera_libre] ;
            num_celdas_ocupadas-- ;
            cout << "Buffer va a enviar valor " << valor << endl ;
            prod_seguidos = 0; //reestablecemos contador de productores atendidos seguidos
            MPI_Ssend( &valor, 1, MPI_INT, estado.MPI_SOURCE, etiq_cons, MPI_COMM_WORLD);
            break;
      }
   }
}

// ---------------------------------------------------------------------

int main( int argc, char *argv[] )
{
   int id_propio, num_procesos_actual, id;

   // inicializar MPI, leer identif. de proceso y número de procesos
   MPI_Init( &argc, &argv );
   MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
   MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );

   if ( num_procesos_esperado == num_procesos_actual )
   {
      // ejecutar la operación apropiada a 'id_propio'
      if ( 0 <= id_propio && id_propio <= num_cons-1 ){ //es consumidor (consumidores del 0 al 3)
         funcion_consumidor(id_propio);
      }
      else if (id_propio == id_buffer){                   //es buffer (id_buffer)
         funcion_buffer();
      } 
      else {                                             //es productor (productores del 4 al 9) hay 6
         id = id_propio - num_cons;                      // 5 ------> 1 = 5 - 4 
         funcion_productor(id);
      }   
   }
   else
   {
      if ( id_propio == 0 ) // solo el primero escribe error, indep. del rol
      { cout << "el número de procesos esperados es:    " << num_procesos_esperado << endl
             << "el número de procesos en ejecución es: " << num_procesos_actual << endl
             << "(programa abortado)" << endl ;
      }
   }

   cout << "-----------------------------------------------------------------" << endl;
   if ( 0 <= id_propio && id_propio <= num_cons-1 ){ //es consumidor (consumidores del 0 al 3)
      cout << "Hebra " << id_propio << " CONSUMIDOS:" << cont_cons_hebra[id_propio]  << endl;
   }
   else if (id_propio != id_buffer){                  //es productor (productores del 4 al 9)
      cout << "Hebra " << id_propio << " PRODUCIDOS:" << cont_prod_hebra[id] << endl;
   } 
   cout << "-----------------------------------------------------------------" << endl;


   
    // al terminar el proceso, finalizar MPI
   MPI_Finalize( );
  
   return 0;
}

/*
Construir un programa MPI, que se deberá llamar prod_cons_ex.cpp, con operaciones de
paso de mensaje síncronas siguiendo el esquema del productor-consumidor, en el que
tenemos cuatro procesos consumidores (identificadores del 0 al 3) y seis procesos
productores (identificadores del 4 al 9) que producirán 15 elementos cada uno. Ambos tipos
de procesos se comunican mediante un proceso intermedio “buffer” con identificador 10.
Dicho proceso intermedio gestiona un vector de tamaño 10. Otras consideraciones:

- La gestión del vector se debe hacer en modo LIFO, es decir, se debe consumir el último
elemento producido.

- El proceso buffer trata de equilibrar la atención a productores y consumidores
independientemente de la ocupación del vector. Por tanto, cada vez que atiende a cuatro
productores de forma consecutiva (sin atender entre medias a ningún consumidor), no
atiende más peticiones de los productores hasta que no haya atendido a un proceso
consumidor

*/
