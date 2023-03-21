// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Practica 2. Introducción a los monitores en C++11.
// Alumno: Alberto Llamas Gonzalez
// Archivo: fumadores.cpp
// -----------------------------------------------------------------------------------
//Ejecucion: g++ -std=c++11 -pthread -Wfatal-errors -o fumadores fumadores.cpp scd.cpp


#include <iostream>
#include <iomanip>
#include <cassert>
#include <random>
#include <thread>
#include "scd.h"

using namespace std;
using namespace scd;

const int num_fumadores = 3;
mutex write_mtx;                //cerrojo para cout
//-------------------------------------------------------------------------
// Función que simula la acción de producir un ingrediente, como un retardo
// aleatorio de la hebra (devuelve número de ingrediente producido)

int producir_ingrediente()
{
   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_produ( aleatorio<10,100>() );

   // informa de que comienza a producir
   write_mtx.lock();
   cout << "Estanquero : empieza a producir ingrediente (" << duracion_produ.count() << " milisegundos)" << endl;
   write_mtx.unlock();
   // espera bloqueada un tiempo igual a ''duracion_produ' milisegundos
   this_thread::sleep_for( duracion_produ );

   const int num_ingrediente = aleatorio<0,num_fumadores-1>() ;

   // informa de que ha terminado de producir
   write_mtx.lock();
   cout << "Estanquero : termina de producir ingrediente " << num_ingrediente << endl;
   write_mtx.unlock();
   return num_ingrediente ;
}

//-------------------------------------------------------------------------
// Función que simula la acción de fumar, como un retardo aleatoria de la hebra

void fumar( int num_fumador )
{

   // calcular milisegundos aleatorios de duración de la acción de fumar)
   chrono::milliseconds duracion_fumar( aleatorio<20,200>() );

   // informa de que comienza a fumar
    write_mtx.lock();
    cout << "Fumador " << num_fumador << "  :"
          << " empieza a fumar (" << duracion_fumar.count() << " milisegundos)" << endl;
    write_mtx.unlock();
   // espera bloqueada un tiempo igual a ''duracion_fumar' milisegundos
   this_thread::sleep_for( duracion_fumar );

   // informa de que ha terminado de fumar
    write_mtx.lock();
    cout << "Fumador " << num_fumador << "  : termina de fumar, comienza espera de ingrediente." << endl;
    write_mtx.unlock();
}


class Estanco : public HoareMonitor {
   private:
      int ingrediente_producido;         //ingrediente en la mesa del estanco
                                             //colas condicion:
      CondVar mostrador_vacio,                  //que se produzca un ingrediente
               ingr_disp[num_fumadores];        //se produzca un ingrediente i para el fumador i

   
   public:

      Estanco();
      void obtenerIngrediente(int num_fumador);
      void ponerIngrediente(int ingrediente);
      void esperarRecogidaIngrediente();

};

Estanco::Estanco() {
    ingrediente_producido = -1;             // inicialmente no hay nada en el mostrador(no se ha producido ningun ingrediente)
    mostrador_vacio = newCondVar();

   for (int i = 0; i < num_fumadores; i++) 
      ingr_disp[i] = newCondVar();
}

void Estanco::obtenerIngrediente(int num_fumador) {
   if (ingrediente_producido != num_fumador)
        ingr_disp[num_fumador].wait();

    ingrediente_producido = -1;

    cout << "\t\tFumador " << num_fumador << " retira ingrediente: " << num_fumador << endl;

    mostrador_vacio.signal();
}

void Estanco::ponerIngrediente(int ingrediente) {
   ingrediente_producido = ingrediente;
   cout << "Estanquero " << ingrediente << ": disponible ingrediente "
         << ingrediente_producido << endl;


    ingr_disp[ingrediente].signal();
}

void Estanco::esperarRecogidaIngrediente(){
   if (ingrediente_producido != -1) {
      mostrador_vacio.wait();
   }
}


void funcion_hebra_estanqueros(MRef<Estanco> monitor) {
    while (true) {
        int ingrediente = producir_ingrediente();
        monitor->ponerIngrediente(ingrediente);
        monitor->esperarRecogidaIngrediente();
    }
}

void funcion_hebra_fumadores(MRef<Estanco> monitor, int num_fumador) {
    while (true) {
        monitor->obtenerIngrediente(num_fumador);
        fumar(num_fumador);
    }
}

int main() {
    int i;

    cout << "***************************************************" << endl
         << "PROBLEMA DE LOS FUMADORES" << endl
         << "***************************************************" << endl << endl;

    cout << "Número de fumadores: " << num_fumadores << endl
         << flush;

    MRef<Estanco> monitor = Create<Estanco>();
   
    thread hebraEstanquero( funcion_hebra_estanqueros, monitor );
    thread hebra_fumadores[num_fumadores];
    for ( int i = 0; i < num_fumadores; i++ )
        hebra_fumadores[i] = thread(funcion_hebra_fumadores, monitor, i);

    hebraEstanquero.join() ;
    
    
    for ( int i = 0; i < num_fumadores; i++ ) hebra_fumadores[i].join();
}