// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Practica 2. Introducción a los monitores en C++11.
// Alumno: Alberto Llamas Gonzalez
// Archivo: lec_esc_Ejercicio.cpp
// -----------------------------------------------------------------------------------
// Ejecucion: g++ -std=c++11 -pthread -Wfatal-errors -o lec_esc_Ejercicio lec_esc_Ejercicio.cpp scd.cpp

/*
   En clase lo ha hecho asi
   en ini_lectura(){
      if (escrib or contador_lectores == 5) lectura.wait();
      n_lec++;
      if (escritura.queue()) contador_lectores++;
      if (contador_lectores < 5) lectura.signal();
   }

   en fin_lectura(){
      n_lec--;
      if (n_lec == 0){
         contador_lectores = 0;
         escritura.signal();
      }
   }

*/
#include <iostream>
#include <iomanip>
#include <cassert>
#include <random>
#include <thread>
#include "scd.h"

using namespace std;
using namespace scd;

const int num_lectores = 6, num_escritores = 3;

mutex write_mtx;

// SIMULACION LECTURA Y ESCRITURA
//--------------------------------------------------------------------------------------------
void leer(int numLector)
{
   // calcular milisegundos aleatorios de duración de la acción de leer)
   chrono::milliseconds duracion_leer(aleatorio<10, 100>());

   // informa de que comienza a leer
   write_mtx.lock();
   cout << "Lector " << numLector << ": leyendo... (" << duracion_leer.count() << " milisegundos)" << endl;
   write_mtx.unlock();
   // espera bloqueada un tiempo igual a ''duracion_leer' milisegundos
   this_thread::sleep_for(duracion_leer);
}

void escribir(int numEscritor)
{
   // calcular milisegundos aleatorios de duración de la acción de escribir)
   chrono::milliseconds duracion_escribir(aleatorio<10, 100>());

   // informa de que comienza a leer
   write_mtx.lock();
   cout << "Escritor " << numEscritor << ": escribiendo... (" << duracion_escribir.count() << " milisegundos)" << endl;
   write_mtx.unlock();
   // espera bloqueada un tiempo igual a ''duracion_escribir' milisegundos
   this_thread::sleep_for(duracion_escribir);
}
//--------------------------------------------------------------------------------------------
//-------------------------------MONITOR------------------------------------------------------
//--------------------------------------------------------------------------------------------
class Lec_Esc : public HoareMonitor
{
private:
   // Variables permanentes
   unsigned int n_lec; // numero de lectores leyendo
   bool escrib;        // true si hay algun escritor escribiendo
   const int max_lectores = 5;
   unsigned int lec_pueden, e_pueden;
   // colas condicion:
   CondVar lectura;   // no hay escritor escribiendo -> lectura posible
   CondVar escritura; // no hay lector leyendo ni escritor escribiendo -> escritura posible

public:
   Lec_Esc();

   // invocados por lectores
   void ini_lectura(unsigned int num_hebra);
   void fin_lectura(unsigned int num_hebra);

   // invocados por escritores
   void ini_escritura(unsigned int num_hebra);
   void fin_escritura(unsigned int num_hebra);
};

Lec_Esc::Lec_Esc()
{
   escrib = false;
   n_lec = 0;
   lec_pueden = 5;
   lectura = newCondVar();
   escritura = newCondVar();
}

void Lec_Esc::ini_lectura(unsigned int num_hebra)
{
   if (escrib || lec_pueden == 0)
      lectura.wait();

   // Aqui sabemos ya que se puede leer porque lec_pueden > 0 y !escrib (no hay nadie escribiendo)
   cout << " --> (L) hebra " << num_hebra << " empieza a leer" << endl
        << flush;

   n_lec++;

   if (!escritura.empty())
      lec_pueden--;

   if (lec_pueden > 0)
      lectura.signal();
}

void Lec_Esc::fin_lectura(unsigned int num_hebra)
{
   cout << " <-- (L) hebra " << num_hebra << " termina de leer" << endl
        << flush;
   n_lec--;
   if (n_lec <= 0){
      escritura.signal();
   }
}

void Lec_Esc::ini_escritura(unsigned int num_hebra)
{
   if (n_lec > 0 || escrib)
   {
      escritura.wait();
   }
   cout << " --> (E) hebra " << num_hebra << " empieza a escribir" << endl
        << flush;
   escrib = true;
}

void Lec_Esc::fin_escritura(unsigned int num_hebra)
{
   cout << " <-- (E) hebra " << num_hebra << " termina de escribir" << endl
        << flush;

   escrib = false;
   if (!lectura.empty())
   {
      lectura.signal();
   }
   else
      escritura.signal();
}

void funcion_hebra_escritor(MRef<Lec_Esc> monitor, unsigned int numHebra)
{
   while (true)
   {
      chrono::milliseconds retardo(aleatorio<10, 100>());
      monitor->ini_escritura(numHebra);
      this_thread::sleep_for(retardo);
      monitor->fin_escritura(numHebra);
      this_thread::sleep_for(retardo);

   }
}

void funcion_hebra_lector(MRef<Lec_Esc> monitor, int numLector)
{
   while (true)
   {
      chrono::milliseconds retardo(aleatorio<10, 100>());
      monitor->ini_lectura(numLector);
      this_thread::sleep_for(retardo);
      monitor->fin_lectura(numLector);
      this_thread::sleep_for(retardo);

   }
}

int main()
{
   MRef<Lec_Esc> monitor = Create<Lec_Esc>();
   thread escritores[num_escritores];
   thread lectores[num_lectores];

   for (int i = 0; i < num_escritores; i++)
      escritores[i] = thread(funcion_hebra_escritor, monitor, i);

   for (int i = 0; i < num_lectores; i++)
      lectores[i] = thread(funcion_hebra_lector, monitor, i);

   for (int i = 0; i < num_escritores; i++)
      escritores[i].join();

   for (int i = 0; i < num_lectores; i++)
      lectores[i].join();
}