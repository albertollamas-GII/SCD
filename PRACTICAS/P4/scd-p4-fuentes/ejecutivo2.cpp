// -----------------------------------------------------------------------------
// Sistemas Concurrentes y Distribuidos.
// Práctica 4. Implementación de sistemas de tiempo real.
//
// Archivo: ejecutivo2.cpp
// Alumno: Alberto Llamas González (32081180K)
//   Datos de las tareas:
//   ------------
//   Ta.  T    C
//   ------------
//   A  500   100
//   B  500   150
//   C  1000  200
//   D  2000  240
//  -------------
//
//  Planificación (con Ts == 500 ms)
//  *---------*----------*---------*--------*
//  | A B C   | A B D   | A B C   | A B   |
//  *---------*----------*---------*--------*
//  Tm = mcm(500, 500, 1000, 2000) = 2000

/*
1) ¿Cuál es el mínimo tiempo de espera que queda al final de las iteraciones del ciclo secundario con tu solución?

      El tiempo mínimo de espera de cada iteración del ciclo secundario viene dado por el 
      Tmín = (Ts=500ms) - (la sumatoria del tiempo necesario (Ci) para la ejecución de cada tarea en ese ciclo)

        *---------*
        | A B C   | <-- Iteración 1 y 3 (ejecutan las mismas tareas)
        *---------*
      Tmin(1 y 3) = (Ts=500) - (C1=100 + C2=150 + C3=200) = (500) - (450) = 50 ms

        *---------*
        | A B D   | <-- Iteración 2
        *---------*
      Tmin(2) = (Ts=500) - (C1=100 + C2=150 + C4=240) = (500) - (490) = 10 ms

        *-------*
        | A B   | <-- Iteración 4
        *-------*
      Tmin(4) = (Ts=500) - (C1=100 + C2=150) = (500) - (250) = 250 ms



   2) ¿Sería planificable si la tarea D tuviese un tiempo cómputo de 250 ms?

      Debemos fijarnos en la iteración donde se ejecuta la tarea D, es decir en la iteración 2.
      Como el Ts es de 500 ms, si sumamos los tiempos de las 3 tareas que se
      ejecutan en esa iteración, si D necesita 250ms para ejecutarse, entonces el tiempo de 
      espera sería 0.

      Por tanto, teóricamente sí que sería planificable, ya que el tiempo total de la iteración 2
      es igual al Ts.

      Sin embargo, a la hora de ejecutar el programa, hemos visto que se producen retrasos,
      por tanto seguramente en la ejecución va a acabar superando ese Ts.

*/
// -----------------------------------------------------------------------------

#include <string>
#include <iostream> // cout, cerr
#include <thread>
#include <chrono>   // utilidades de tiempo
#include <ratio>    // std::ratio_divide

using namespace std ;
using namespace std::chrono ;
using namespace std::this_thread ;

// tipo para duraciones en segundos y milisegundos, en coma flotante:
typedef duration<float,ratio<1,1>>    seconds_f ;
typedef duration<float,ratio<1,1000>> milliseconds_f ;

// -----------------------------------------------------------------------------
// tarea genérica: duerme durante un intervalo de tiempo (de determinada duración)

void Tarea( const std::string & nombre, milliseconds tcomputo )
{
   cout << "   Comienza tarea " << nombre << " (C == " << tcomputo.count() << " ms.) ... " ;
   sleep_for( tcomputo );
   cout << "fin." << endl ;
}

// -----------------------------------------------------------------------------
// tareas concretas del problema:

void TareaA() { Tarea( "A", milliseconds(100) );  }
void TareaB() { Tarea( "B", milliseconds(150) );  }
void TareaC() { Tarea( "C", milliseconds(200) );  }
void TareaD() { Tarea( "D", milliseconds(240) );  }

// -----------------------------------------------------------------------------
// implementación del ejecutivo cíclico:

int main( int argc, char *argv[] )
{
   // Ts = duración del ciclo secundario (en unidades de milisegundos, enteros)
   const milliseconds Ts_ms( 250 );

   // ini_sec = instante de inicio de la iteración actual del ciclo secundario
   time_point<steady_clock> ini_sec = steady_clock::now();

   while( true ) // ciclo principal
   {
      cout << endl
           << "---------------------------------------" << endl
           << "Comienza iteración del ciclo principal." << endl ;

      for( int i = 1 ; i <= 4 ; i++ ) // ciclo secundario (4 iteraciones)
      {
         cout << endl << "Comienza iteración " << i << " del ciclo secundario." << endl ;

         switch( i )
         {
            case 1 : TareaA(); TareaB(); TareaC();           break ;
            case 2 : TareaA(); TareaB(); TareaD();           break ;
            case 3 : TareaA(); TareaB(); TareaC();           break ;
            case 4 : TareaA(); TareaB();                     break ;
         }

         // calcular el siguiente instante de inicio del ciclo secundario
         ini_sec += Ts_ms ;

         // esperar hasta el inicio de la siguiente iteración del ciclo secundario
         sleep_until( ini_sec );
         
         // Retardo: ACTIVIDAD (ejecutivo1-compr.cpp)
         time_point<steady_clock> tiempo_retraso = steady_clock::now();
         milliseconds_f total = tiempo_retraso- ini_sec;
         cout << "Retraso: " << total.count() << endl;
      }
   }
}
