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
// mpicxx -std=c++11 -o falsifLIFO prodcons2-muLIFOfalsificados.cpp
// mpirun --oversubscribe -np 10 ./falsifLIFO
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
   num_cons              = 8, 
   num_prod_fals         = 2,
   
   id_buffer             = num_prod ,
   etiq_prod             = 1,
   etiq_cons             = 2,

   num_procesos_esperado = num_prod + num_cons + num_prod_fals + 2,   // (+2--> buffer y almacén)
   num_items             = 32,//80,   //se crean 80 prodcutos de cada tipo

   
   producto_fals         = 44,
   id_almacen            = id_buffer+1,  //el id del sig proceso al buffer estándar
   etiq_prod_fals             = 3;

   int k_prod = num_items / num_prod; //num items a producir por cada productora
   int k_cons = num_items / num_cons; //num items a producir por cada productora
   unsigned int cont_prod_hebra[num_prod] = {0}; //contador que indica nº de items producidos por cada hebra
   unsigned int cont_cons_hebra[num_cons] = {0}; //contador que indica nº de items consumidos por cada hebra
   
   int k_prod_fals = num_items / num_prod_fals; //num items falsos a producir por cada productor de falsos
   unsigned int cont_prod_fals_hebra[num_prod_fals] = {0}; //contador que indica nº de items falsos producidos por cada hebra productora de productos falsos


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
// producir valor (siempre es 44 --> producto_fals)

int producir_falso( int num_prod)
{
   sleep_for( milliseconds( aleatorio<10,100>()) );
   cont_prod_fals_hebra[num_prod]++;
   cout << "(FALSO p" << num_prod << ") Productor falso ha PRODUCIDO valor " << producto_fals << endl << flush;
   return producto_fals;
}

// ---------------------------------------------------------------------
          
void funcion_productor_falsificados( int num_prod)
{  
   for ( unsigned int i= 0 ; i < k_prod_fals ; i++ )
   {
      // producir valor (siempre es 44 --> producto_fals)
      int valor_prod = producir_falso( num_prod);
      // enviar valor
      cout << " --> (FALSO p" << num_prod << ") Productor falso va a enviar valor " << valor_prod << endl << flush;
      MPI_Ssend( &valor_prod, 1, MPI_INT, id_almacen, etiq_prod_fals, MPI_COMM_WORLD );
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
   bool producto_fals = false;

   for( unsigned int i=0 ; i < k_cons; i++ )
   {  
      if (num_cons ==  0 && producto_fals){ //si es el consumidor con el id más bajo y le toca consumir falso
         MPI_Ssend( &peticion,  1, MPI_INT, id_almacen, etiq_cons, MPI_COMM_WORLD);
         MPI_Recv ( &valor_rec, 1, MPI_INT, id_almacen, etiq_cons, MPI_COMM_WORLD,&estado );
         producto_fals = false; //en la sig iteración no toca consumir producto falso
         cout << "(FALSO c" << num_cons << ") Consumidor ha COMPRADO PRODUCTO FALSIFICADO " << endl << flush ;

      }else{
         MPI_Ssend( &peticion,  1, MPI_INT, id_buffer, etiq_cons, MPI_COMM_WORLD);
         MPI_Recv ( &valor_rec, 1, MPI_INT, id_buffer, etiq_cons, MPI_COMM_WORLD,&estado );
         cout << " <-- (c" << num_cons << ") Consumidor ha recibido valor " << valor_rec << endl << flush ;

         if(num_cons == 0)
            producto_fals = true; //en la sig iteración toca consumir producto falso
         
      }

      consumir( valor_rec, num_cons);
      
   }

   //solo falta saber qué proceso más, a parte del 0 consume del falsificador,
   // puesto que se deben consumir todos los valores del falsificador, para que no se quede bloqueado
   // esperando por peticiciones para consumir que nunca llegarán
   

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
         cout << "******** BUFFER LLENO ********" << endl ;
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

void funcion_almacen()
{
   int        almacen[tam_vector],      // buffer con celdas ocupadas y vacías
              valor,                   // valor recibido o enviado
              primera_libre       = 0, // índice de primera celda libre
              num_celdas_ocupadas = 0, // número de celdas ocupadas
              etiq_emisor_aceptable;    // etiqueta de emisor aceptable
   MPI_Status estado ;                 // metadatos del mensaje recibido

   for( unsigned int i=0 ; i < num_items*2; i++ )
   {  
      // 1. determinar si puede enviar solo prod., solo cons, o todos
      if ( num_celdas_ocupadas == 0 ) {// si buffer vacío
         etiq_emisor_aceptable = etiq_prod_fals ;          // $~~~$ solo prod. falso
      }  
      else if ( num_celdas_ocupadas == tam_vector ){// si buffer lleno
         cout << "------ ALMACÉN LLENO ------" << endl ;
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
         case etiq_prod_fals: // si ha sido el productor: insertar en buffer
            almacen[primera_libre] = valor ;
            primera_libre++;
            num_celdas_ocupadas++ ;
            cout << "Almacén ha recibido valor " << valor << endl ;
            break;

         case etiq_cons: // si ha sido el consumidor: extraer y enviarle
            primera_libre--;
            //cout << "PRIM_LIBRE_CONS: " << primera_libre << endl ;
            valor = almacen[primera_libre] ;
            num_celdas_ocupadas-- ;
            cout << "Almacén va a enviar valor " << valor << endl ;
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
      
      //id productores van de 0 a num_prod -1 
      if ( 0 <= id_propio && id_propio <= num_prod-1 ){ //es productor
         funcion_productor(id_propio);
      }
      else if (id_propio == id_buffer){                   //es buffer
         funcion_buffer();
      } 
      else if (id_propio == id_almacen){                   //es almacen de prods falsos
         funcion_almacen();
      } 
      else if( id_almacen+1 <= id_propio && id_propio <= id_almacen + num_cons){     //es consumidor
      //id consumidores van de num_prod+2 a num_prod+num_cons
         id = id_propio - num_prod - 2; 
         funcion_consumidor(id);
      }   
      else{ //los procesos con los 2 últimos id
         //por último tenemos a los productores de productos falsificados
         id = id_propio - num_prod - 2 - num_cons; 
         funcion_productor_falsificados(id);
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
   else if (id_almacen+1 <= id_propio && id_propio <= id_almacen + num_cons){     //es consumidor
      cout << "Hebra " << id_propio << " CONSUMIDOS:" << cont_cons_hebra[id]  << endl;
   } 
   else if (id_propio != id_buffer && id_propio != id_almacen){     //es productor de prod. falsos
      cout << "Hebra " << id_propio << " PRODUCIDOS FALSOS:" << cont_prod_fals_hebra[id]  << endl;
   } 
   cout << "-----------------------------------------------------------------" << endl;

   
    // al terminar el proceso, finalizar MPI
   MPI_Finalize( );
  
   return 0;
}

/*


Ejercicio para ser realizado sobre el problema de varios productores y consumidores con buffer
de los ejercicios de la práctica 3

- Se añade al problema el papel de productor de productos falsificados. Puede haber un
número arbitrario de productores de este tipo, pero para el examen esta cantidad se fijará a 2.
El producto falsificado siempre es el valor 44.

- En total, los productores de productos falsificados producen la misma cantidad de productos
falsos que los productores estándar.

- Se añade al problema el papel de almacén de productos falsificados. Este almacén será un
buffer y tendrá el mismo tamaño que el buffer del que ya dispones. A este almacén solo
añaden productos los nuevos productores de productos falsificados. En el resto de los aspectos
se comporta como el buffer que explica el guion de la práctica.

- Tanto el buffer estándar como el nuevo de productos falsificados serán de tipo LIFO.

- Ambos buffers tienen por tanto el mismo comportamiento y solo se diferencian en quienes
son los procesos que introducen/sacan elementos de los mismos.

- Se modifica el comportamiento de uno de los consumidores, concretamente del que tenga el
identificador más bajo. Este consumidor irá consumiendo productos estándar y falsificados de
forma alternada (alternando el tipo de producto en cada iteración).

- El consumidor correspondiente informará que ha comprado el producto falsificado solo
mediante un mensaje en la consola.

Fijar el número de productores estándar a 4, el de consumidores a 8 y el número total de
productos creados será de 80 (80 de cada tipo).

Incluir al inicio del código fuente un comentario con las ordenes que hay que utilizar para
poder compilar y ejecutar correctamente este ejercicio.
*/
