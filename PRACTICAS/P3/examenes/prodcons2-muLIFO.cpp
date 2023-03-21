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
// mpicxx -std=c++11 -o prodcons2-muLIFO prodcons2-muLIFO.cpp
// mpirun --oversubscribe -np 10 ./prodcons2-muLIFO
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

   num_prod              = 4,
   num_cons              = 5,
   id_buffer             = num_prod , //buffer es el proceso entre num_prod y num_cons
   etiq_prod             = 1,
   etiq_cons             = 2,
   num_procesos_esperado = num_prod + num_cons + 1,   // (+1 --> buffer)
   num_items             = num_prod * num_cons;

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

   for( unsigned int i=0 ; i < num_items*2 ; i++ )
   {
      // 1. determinar si puede enviar solo prod., solo cons, o todos
      if ( num_celdas_ocupadas == 0 ) {// si buffer vacío
         etiq_emisor_aceptable = etiq_prod ;          // $~~~$ solo prod.
      }  
      else if ( num_celdas_ocupadas == tam_vector ){// si buffer lleno
         //cout << "******** BUFFER LLENO ********" << endl ;
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
            break;

         case etiq_cons: // si ha sido el consumidor: extraer y enviarle
            primera_libre--;
            //cout << "PRIM_LIBRE_CONS: " << primera_libre << endl ;
            valor = buffer[primera_libre] ;
            num_celdas_ocupadas-- ;
            cout << "Buffer va a enviar valor " << valor << endl ;
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
      if ( 0 <= id_propio && id_propio <= num_prod-1 ){ //es productor
         funcion_productor(id_propio);
      }
      else if (id_propio == id_buffer){                   //es buffer
         funcion_buffer();
      } 
      else {                                             //es consumidor
         id = id_propio - num_prod - 1; 
         funcion_consumidor(id);
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
   if ( 0 <= id_propio && id_propio <= num_prod-1 ){ //es productor
      cout << "Hebra " << id_propio << " PRODUCIDOS:" << cont_prod_hebra[id_propio] << endl;
   }
   else if (id_propio != id_buffer){                  //es consumidor
      cout << "Hebra " << id_propio << " CONSUMIDOS:" << cont_cons_hebra[id]  << endl;
   } 
   cout << "-----------------------------------------------------------------" << endl;



   
    // al terminar el proceso, finalizar MPI
   MPI_Finalize( );
  
   return 0;
}
