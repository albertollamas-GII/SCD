// g++ -std=c++11 -pthread -I. -o prodconsFIFO prodcons-FIFO.cpp scd.cpp
#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "scd.h"

using namespace std ;
using namespace scd ;

//**********************************************************************

//**********************************************************************
// Variables globales

const unsigned 
   num_items = 40 ,   // número de items
	tam_vec   = 10 ;   // tamaño del buffer
unsigned  
    //inicializamos los vectores a 0 (todos los elementos del vector tienen valor 0)
   cont_prod[num_items] = {0}, // contadores de verificación: para cada dato, número de veces que se ha producido.
   cont_cons[num_items] = {0}, // contadores de verificación: para cada dato, número de veces que se ha consumido.
   siguiente_dato       = 0 ;  // siguiente dato a producir en 'producir_dato' (solo se usa ahí)

   //añadido:
   int v_intermedio[tam_vec];
   int primera_libre = 0; //índice en el vector de la primera celda libre
   int primera_ocupada = 0; //índice en el vector de la primera celda ocupada
   Semaphore libres = tam_vec; //num entradas libres (k+#L-#E)
   Semaphore ocupadas = 0; //num entradas ocupadas (#E-#L)

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

unsigned producir_dato()
{
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));
   const unsigned dato_producido = siguiente_dato ;
   siguiente_dato++ ;
   cont_prod[dato_producido] ++ ; 
   cout << "producido: " << dato_producido << endl << flush ;
   return dato_producido ;
}
//----------------------------------------------------------------------

void consumir_dato( unsigned dato )
{
   assert( dato < num_items ); //Si la condición no es true se produce aserción
   cont_cons[dato] ++ ;
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

   cout << "                  consumido: "<< dato << endl ;

}

//----------------------------------------------------------------------
// Función para imprimir el buffer
void print_buffer(){
      cout << "-----------------------------------------------------------------" << endl
        << "Buffer: ";
   for(int i=0; i < tam_vec; i++) {
      cout << v_intermedio[i];
      if (primera_libre == i || primera_ocupada == i){
         if (primera_ocupada == i)
         cout << "b"; //busy
         if (primera_libre == i)
            cout << "f"; //free
      }
      cout << "  ";
   }
   cout << endl
        << "------------------------------------------------------------------" << endl;
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

void  funcion_hebra_productora(  )
{
   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      int dato = producir_dato() ;
      sem_wait(libres) ;
      v_intermedio[primera_libre] = dato;
      primera_libre++;
      primera_libre = primera_libre % tam_vec;
      sem_signal (ocupadas) ;
      cout << " --> Insertado dato en  buffer:  " << dato <<" ##" << endl;
      //print_buffer();
   }
}

//----------------------------------------------------------------------

void funcion_hebra_consumidora(  )
{
   for( unsigned i = 0 ; i < num_items ; i++ )
   {
      int dato ;
      sem_wait(ocupadas);
      dato = v_intermedio[primera_ocupada];
      primera_ocupada++;
      primera_ocupada = primera_ocupada % tam_vec;
      sem_signal(libres);
      cout << " <-- consumido dato en  buffer: " << dato << " ##"<< endl;

      consumir_dato( dato ) ;
      //print_buffer();
    }
}
//----------------------------------------------------------------------

int main()
{
   cout << "-----------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (solución FIFO)." << endl
        << "------------------------------------------------------------------" << endl
        << flush ;
   
   //print_buffer();

   thread hebra_productora ( funcion_hebra_productora ),
          hebra_consumidora( funcion_hebra_consumidora );

   hebra_productora.join() ;
   hebra_consumidora.join() ;

   test_contadores();

   cout << "-----------------------------------------------------------------" << endl
        << "Fin del programa" << endl
        << "------------------------------------------------------------------" << endl;

}
