
//**********************************************************************
// Compilación y ejecución
// g++ -std=c++11 -pthread -I. -o lec_esc lec_esc.cpp scd.cpp
// ./lec_esc
//**********************************************************************

#include <iostream>
#include <iomanip>
#include <cassert>
#include <random>
#include <thread>
#include "scd.h"

using namespace std ;
using namespace scd ;

const int num_hlectoras = 4;
const int num_hescritoras = 2;

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

void retraso(){
   this_thread::sleep_for( chrono::milliseconds( aleatorio<10,25>() ));
}

// *****************************************************************************
// clase para monitor 
class Lec_Esc : public HoareMonitor
{
 private:
   int n_lec;                //mumero de lectores leyendo
   bool escrib;              //true si hay algún escritor escribiendo

 CondVar                    // colas condicion:
   lectura,                 //  no hay escrit. escribiendo, lectura posible
   escritura ;               //  no hay lect. ni escrit., escritura posible 

 public:                    // constructor y métodos públicos
   Lec_Esc() ;             // constructor
   void  ini_lectura(unsigned int num_hebra), fin_lectura(unsigned int num_hebra); //invocados por lectores
   void  ini_escritura(unsigned int num_hebra), fin_escritura(unsigned int num_hebra); //invocados por escritores
} ;

// -----------------------------------------------------------------------------

Lec_Esc::Lec_Esc()
{
   n_lec = 0 ;
   escrib = false;
   lectura      = newCondVar();
   escritura        = newCondVar();
}
// -----------------------------------------------------------------------------
void Lec_Esc::ini_lectura( unsigned int num_hebra )
{
   if (escrib)          //si hay escritor
      lectura.wait();   //esperar
   cout << " --> (L) hebra " << num_hebra << " empieza a leer" << endl << flush ;
   n_lec++;             //registrar un lector más
   lectura.signal();    //desbloqueo en caden de posibles lectores bloqueados
}

// -----------------------------------------------------------------------------
void Lec_Esc::fin_lectura( unsigned int num_hebra )
{  
   cout << " <-- (L) hebra " << num_hebra << " termina de leer" << endl << flush ;
   n_lec--;             //registrar un lector menos
   if(n_lec == 0)       // si es el último lector
      escritura.signal(); //desbloquear un escritor
}

// -----------------------------------------------------------------------------
void Lec_Esc::ini_escritura( unsigned int num_hebra )
{
   if ( (n_lec > 0) || escrib )   //si hay otro(s), esperar
      escritura.wait();   
   cout << " --> (E) hebra " << num_hebra << " empieza a escribir" << endl << flush ;
   escrib = true;                   //registrar que hay escritor
}

// -----------------------------------------------------------------------------
void Lec_Esc::fin_escritura( unsigned int num_hebra )
{  
   cout << " <-- (E) hebra " << num_hebra << " termina de escribir" << endl << flush ;
   escrib = false;             //registrar que ya no hay escritor
   if (!lectura.empty())      //si hay lectores
      lectura.signal();       //despeetamos uno
   else
      escritura.signal();      //si no, despertamos un escritor
}


// *****************************************************************************
// funciones de hebras

void funcion_hebra_lectora( MRef<Lec_Esc> monitor, unsigned int num_hebra )
{
   while(true)
   {
      monitor->ini_lectura(num_hebra);
      retraso();
      monitor->fin_lectura(num_hebra);
      retraso();
   }
}
// -----------------------------------------------------------------------------

void funcion_hebra_escritora( MRef<Lec_Esc> monitor, unsigned int num_hebra )
{
   while(true)
   {
      monitor->ini_escritura(num_hebra);
      retraso();
      monitor->fin_escritura(num_hebra);
      retraso();
   }
}
// -----------------------------------------------------------------------------

int main()
{
   cout << "--------------------------------------------------------------------" << endl
        << "Problema de los Lectores/Escritores. " << endl
        << "--------------------------------------------------------------------" << endl
        << flush ;

   // crear monitor  ('monitor' es una referencia al mismo, de tipo MRef<...>)
   MRef<Lec_Esc> monitor = Create<Lec_Esc>() ;

   // crear y lanzar las hebras
   thread hebras_escritoras[num_hescritoras];
   thread hebras_lectoras[num_hlectoras];

   for(int i=0; i < num_hescritoras; i++) {
      hebras_escritoras[i] = thread(funcion_hebra_escritora, monitor, i);
   }

   for(int i=0; i < num_hlectoras; i++) {
      hebras_lectoras[i] = thread(funcion_hebra_lectora, monitor, i);
   }

   // esperar a que terminen las hebras
   for(int i=0; i < num_hescritoras; i++) {
      hebras_escritoras[i].join() ;
   }

   for(int i=0; i < num_hlectoras; i++) {
      hebras_lectoras[i].join() ;
   }

}
