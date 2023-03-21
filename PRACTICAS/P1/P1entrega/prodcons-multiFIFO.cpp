// g++ -std=c++11 -pthread -o multiFIFO prodcons-multiFIFO.cpp scd.cpp
#include <iostream>
#include <cassert>
#include <thread>
#include <mutex>
#include <random>
#include "scd.h"

using namespace std ;
using namespace scd ;

//**********************************************************************
// Compilación y ejecución
// ./multiFIFO

//**********************************************************************
// Variables globales

const unsigned 
   num_items = 60 ,   // número de items
	tam_vec   = 10 ;   // tamaño del buffer
unsigned  
    //inicializamos los vectores a 0 (todos los elementos del vector tienen valor 0)
   cont_prod[num_items] = {0}, // contadores de verificación: para cada dato, número de veces que se ha producido.
   cont_cons[num_items] = {0}; // contadores de verificación: para cada dato, número de veces que se ha consumido.
   //siguiente_dato       = 0 ;  // siguiente dato a producir en 'producir_dato' (solo se usa ahí)

   //añadido:
   int v_intermedio[tam_vec];
   int primera_libre = 0; //índice en el vector de la primera celda libre
   int primera_ocupada = 0; //índice en el vector de la primera celda ocupada
   Semaphore libres = tam_vec; //num entradas libres (k+#L-#E)
   Semaphore ocupadas = 0; //num entradas ocupadas (#E-#L)

   //ejer multi
   Semaphore em_p = 1; //se ocupará de la exclusión mutua entre hebras productoras
   Semaphore em_c = 1; //se ocupará de la exclusión mutua entre hebras consumidoras
   const unsigned num_hproductoras = 2;
   const unsigned num_hconsumidoras = 3;
   unsigned cont_prod_hebra[num_hproductoras] = {0}; //contador que indica nº de items producidos por cada hebra
   unsigned cont_cons_hebra[num_hconsumidoras] = {0}; //contador que indica nº de items consumidos por cada hebra
   
   unsigned p_prod = num_items / num_hproductoras;
   unsigned p_cons = num_items / num_hconsumidoras;


//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

unsigned producir_dato(unsigned int num_hebra)
{
   this_thread::sleep_for(chrono::milliseconds(aleatorio<20, 100>()));
   //const unsigned dato_producido = siguiente_dato;
   //siguiente_dato++;
   const unsigned dato_producido = num_hebra * p_prod + cont_prod_hebra[num_hebra];
   cont_prod[dato_producido]++;
   cont_prod_hebra[num_hebra]++; //incrementamos nº items producidos por la hebra actual
   cout << "La hebra " << num_hebra << " ha producido: " << dato_producido << endl;
         cout << " --- Número de datos producidos por la hebra "<< num_hebra << ": " <<cont_prod_hebra[num_hebra] << "--- "<<endl;
   return dato_producido;
}

//----------------------------------------------------------------------

void consumir_dato( unsigned dato, unsigned int num_hebra)
{
   assert( dato < num_items ); //Si la condición no es true se produce aserción
   cont_cons[dato] ++ ;
   cont_cons_hebra[num_hebra]++; //incrementamos nº items consumidos por la hebra actual
   this_thread::sleep_for( chrono::milliseconds( aleatorio<20,100>() ));

   cout << "La hebra " << num_hebra << " ha consumido: " << dato << endl;

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

void  funcion_hebra_productora( unsigned int num_hebra )
{  
   for( unsigned i = 0; i < p_prod; i++ )
   {  
      int dato = producir_dato(num_hebra);
      sem_wait(libres) ;
      sem_wait(em_p);
      v_intermedio[primera_libre] = dato;
      primera_libre++;
      primera_libre = primera_libre % tam_vec;
      sem_signal(em_p);
      cout << " --> Insertado dato en  buffer:  " << dato <<" ##" << endl;
      //print_buffer();
      sem_signal (ocupadas) ;
      
   }
}

//----------------------------------------------------------------------

void funcion_hebra_consumidora(unsigned int num_hebra )
{
   for( unsigned i = 0; i < p_cons; i++ )
   {
      int dato ;
      sem_wait(ocupadas);
      sem_wait(em_c);
      dato = v_intermedio[primera_ocupada];
      primera_ocupada++;
      primera_ocupada = primera_ocupada % tam_vec;
      cout << " <-- Leído dato en  buffer: " << dato << " ##"<< endl;
      //print_buffer();
      sem_signal(em_c);
      cout << endl << endl;
      sem_signal(libres);
      consumir_dato( dato, num_hebra) ;
      
    }
}

//----------------------------------------------------------------------

int main()
{
   cout << "-----------------------------------------------------------------" << endl
        << "Problema de los productores-consumidores (solución FIFO)." << endl
        << "------------------------------------------------------------------" << endl
        << flush ;

   thread hebras_productoras[num_hproductoras];
   thread hebras_consumidoras[num_hconsumidoras];

   print_buffer();
   for(int i=0; i < num_hproductoras; i++) {
      hebras_productoras[i] = thread(funcion_hebra_productora,i);
   }

   for(int i=0; i < num_hconsumidoras; i++) {
      hebras_consumidoras[i] = thread(funcion_hebra_consumidora,i);
   }

   for(int i=0; i < num_hproductoras; i++) {
      hebras_productoras[i].join() ;
   }

   for(int i=0; i < num_hconsumidoras; i++) {
      hebras_consumidoras[i].join() ;
   }
   

   test_contadores();

   

   cout << "-----------------------------------------------------------------" << endl
        << "Fin del programa. Resumen del programa:" << endl
        << "------------------------------------------------------------------" << endl;

   cout << "Primera libre: " << primera_libre << " (debe ser 0 por buffer vacío)" << endl;


   cout << "Contador nº de datos producidos por cada hebra: ";
   for(int i=0; i < num_hproductoras; i++) {
      cout << cont_prod_hebra[i] << "  ";
   }
   cout << endl;
   
   cout << "Contador nº de datos consumidos por cada hebra: ";
   for(int i=0; i < num_hconsumidoras; i++) {
      cout << cont_cons_hebra[i] << "  ";
   }

   cout << endl;

}
