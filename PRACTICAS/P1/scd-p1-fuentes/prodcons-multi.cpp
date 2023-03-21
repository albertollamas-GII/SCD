#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "scd.h"

using namespace std ;
using namespace scd ;
//Ejecucion -->  g++ -std=c++11 -pthread -o prodcons-multi_exe prodcons-multi.cpp scd.cpp
//**********************************************************************
// Variables globales

const unsigned 
   num_items = 40 ,   // número de items
	tam_vec   = 10 ;   // tamaño del buffer
unsigned  
   cont_prod[num_items] = {0}, // contadores de verificación: para cada dato, número de veces que se ha producido.
   cont_cons[num_items] = {0}, // contadores de verificación: para cada dato, número de veces que se ha consumido.
   siguiente_dato       = 0 ;  // siguiente dato a producir en 'producir_dato' (solo se usa ahí)
const int num_consumidores = 3, num_productores = 2; //numero de productores y consumidores
//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

unsigned producir_dato(int num_hebra)
{
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   const unsigned dato_producido = siguiente_dato ;
   siguiente_dato++ ;
   cont_prod[dato_producido] ++ ;
   cout << "producido: " << dato_producido << " (por hebra: " << num_hebra << ")" << endl << flush ;
   return dato_producido ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato, int num_hebra )
{
   //Se puede hacer exclusion mutua aqui con producir dato para evitar que se entrelacen en la ejecucionn
   
   assert( dato < num_items );
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

   cout << "                  consumido: " << dato  << " (por hebra: " << num_hebra << ")" << endl ;

}


//----------------------------------------------------------------------

void test_contadores()
{
   bool ok = true ;
   cout << "comprobando contadores ...." ;
   for( unsigned i = 0 ; i < num_items ; i++ )
   {  if ( cont_prod[i] != 1 )
      {  cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl ;
         ok = false ;
      }
      if ( cont_cons[i] != 1 )
      {  cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl ;
         ok = false ;
      }
   }
   if (ok)
      cout << endl << flush << "solución (aparentemente) correcta." << endl << flush ;
}

//----------------------------------------------------------------------
Semaphore libres(tam_vec), ocupadas(0), EM(1);
int vec[tam_vec];
atomic<int> primera_libre, primera_ocupada;

void  funcion_hebra_productora( int num_hebra )
{
    //FIFO
//    for( unsigned i = num_hebra ; i < num_items ; i+= num_productores )
//    {
//       int dato = producir_dato(num_hebra) ;
//       // completar ........
//       libres.sem_wait();
//       EM.sem_wait();
//       vec[primera_ocupada] = dato;
//       int sig_pos = (primera_ocupada + 1) % tam_vec;
//       primera_ocupada = sig_pos;
//       EM.sem_signal();
//       ocupadas.sem_signal();
//    }

//    LIFO
   for( unsigned i = num_hebra ; i < num_items ; i+= num_productores )
   {
      int dato = producir_dato(num_hebra) ;
      // completar ........
      libres.sem_wait();
      EM.sem_wait();
      vec[primera_libre] = dato;
      primera_libre++;
      EM.sem_signal();
      ocupadas.sem_signal();
   }
}

//----------------------------------------------------------------------

void funcion_hebra_consumidora( int num_hebra )
{
   //FIFO
//    for( unsigned i = num_hebra ; i < num_items ; i+= num_consumidores )
//    {
//       int dato ;
//       ocupadas.sem_wait();
//       dato = vec[primera_libre];
//       primera_libre = (primera_libre + 1) % tam_vec;
//       libres.sem_signal();

//       consumir_dato( dato , num_hebra) ;
//     }

    //LIFO
    for( unsigned i = num_hebra ; i < num_items ; i+= num_consumidores )
    {
      int dato ;
      ocupadas.sem_wait();
      EM.sem_wait();
      primera_libre--;
      dato = vec[primera_libre];
      EM.sem_signal();
      libres.sem_signal();

      consumir_dato( dato, num_hebra ) ;
    }
}
//----------------------------------------------------------------------

int main()
{
   cout << "-----------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (solución LIFO o FIFO)." << endl
        << "------------------------------------------------------------------" << endl
        << flush ;

   thread hebra_productora[num_productores], hebra_consumidora[num_consumidores];

    for (int i = 0; i < num_consumidores; i++) {
        hebra_consumidora[i] = thread(funcion_hebra_consumidora, i);
    }
    for (int i = 0; i < num_productores; i++){
        hebra_productora[i] = thread (funcion_hebra_productora, i);
    }

    for (int i = 0; i < num_consumidores; i++) hebra_consumidora[i].join();
    for (int i = 0; i < num_productores; i++) hebra_productora[i].join();

   

   test_contadores();
}
