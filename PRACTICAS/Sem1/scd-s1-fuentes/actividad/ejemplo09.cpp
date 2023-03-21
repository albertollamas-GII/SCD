// -----------------------------------------------------------------------------
// Sistemas concurrentes y Distribuidos.
// Seminario 1. Programación Multihebra y Semáforos.
//
// Ejemplo 9 (ejemplo9.cpp)
// Calculo concurrente de una integral. Plantilla para completar.
// Alumno: Alberto Llamas Gonzalez
//
// Historial:
// Creado en Abril de 2017
// -----------------------------------------------------------------------------


//indice_comienzo = ih * Bs
//indice_final = min(m-1, (ih + 1) * Bs -1)
//Ciclico -> inicio ih + n limite m


#include <iostream>
#include <iomanip>
#include <chrono>  // incluye now, time\_point, duration
#include <future>
#include <vector>
#include <cmath>

using namespace std ;
using namespace std::chrono;

const long m  = 1024l*1024l*1024l, // número de muestras (del orden de mil millones)
           n  = 4  ;               // número de hebras concurrentes (divisor de 'm')


// -----------------------------------------------------------------------------
// evalua la función $f$ a integrar ($f(x)=4/(1+x^2)$)
double f( double x )
{
  return 4.0/(1.0+x*x) ;
}
// -----------------------------------------------------------------------------
// calcula la integral de forma secuencial, devuelve resultado:
double calcular_integral_secuencial(  )
{
   double suma = 0.0 ;                        // inicializar suma
   for( long j = 0 ; j < m ; j++ )            // para cada $j$ entre $0$ y $m-1$:
   {  const double xj = double(j+0.5)/m ;     //      calcular $x_j$
      suma += f( xj );                        //      añadir $f(x_j)$ a la suma actual
   }
   return suma/m ;                            // devolver valor promedio de $f$
}

// -----------------------------------------------------------------------------
// función que ejecuta cada hebra: recibe $i$ ==índice de la hebra, ($0\leq i<n$)
double funcion_hebraContiguos( long i )
{
   double suma = 0.0;
   for ( long k = (m/n) * i; k < (m/n) * (i+1); k++ )
      suma += f( (k+double(0.5))/m );
   return suma/m;
}

double funcion_hebraCiclico( long i )
{
    double suma = 0.0;
    for ( long int k = i; k < m; k+=n )
    suma += f( (k+double(0.5))/m );
    return suma/m;
}

// -----------------------------------------------------------------------------
// calculo de la integral de forma concurrente
double calcular_integral_concurrente(double (*funcion_hebra)(long i) )
{
  future<double> futuros[n];
  double suma = 0.0;

  for (long int k = 0; k < n; k++) 
   futuros[k] = async(launch::async, funcion_hebraContiguos, k);

  for (long int k = 0; k < n; k++)
   suma += futuros[k].get();

  
  return suma;
}
// -----------------------------------------------------------------------------

int main()
{

  time_point<steady_clock> inicio_sec  = steady_clock::now() ;
  const double             result_sec  = calcular_integral_secuencial(  );
  time_point<steady_clock> fin_sec     = steady_clock::now() ;
  duration<float,milli>    tiempo_sec  = fin_sec  - inicio_sec;

  double x = sin(0.4567);
  time_point<steady_clock> inicio_cont = steady_clock::now() ;
  const double             result_cont = calcular_integral_concurrente(funcion_hebraContiguos);
  time_point<steady_clock> fin_cont    = steady_clock::now() ;
  duration<float,milli>    tiempo_cont = fin_cont - inicio_cont ;

  time_point<steady_clock> inicio_cicl = steady_clock::now() ;
  const double             result_cicl = calcular_integral_concurrente(funcion_hebraCiclico);
  time_point<steady_clock> fin_cicl    = steady_clock::now() ;
  duration<float,milli>   tiempo_cicl = fin_cicl - inicio_cicl ;

  const float              porc        = 100.0*tiempo_cont.count()/tiempo_sec.count() ;
  const float              porc2       = 100.0*tiempo_cicl.count()/tiempo_sec.count() ;

  constexpr double pi = 3.14159265358979323846l ;

  cout << "Número de muestras (m)                                           : " << m << endl
       << "Número de hebras (n)                                             : " << n << endl
       << setprecision(18)
       << "Valor de PI                                                      : " << pi << endl
       << "Resultado secuencial                                             : " << result_sec  << endl
       << "Resultado concurrente con asignacion por bloques contiguos       : " << result_cont << endl
       << "Resultado concurrente con asignacion ciclica                     : " << result_cicl << endl
       << setprecision(5)
       << "Tiempo secuencial                                                : " << tiempo_sec.count()  << " milisegundos. " << endl
       << "Tiempo concurrente con asignacion por bloques contiguos          : " << tiempo_cont.count() << " milisegundos. " << endl
       << "Tiempo concurrente con asignacion ciclica                        : " << tiempo_cicl.count() << " milisegundos. " << endl
       << setprecision(4)
       << "Porcentaje t.conc/t.sec.(bloques contiguos)                      : " << porc << "%" << endl
       << "Porcentaje t.conc/t.sec.(asignacion ciclica)                     : " << porc2 << "%" << endl
       << endl << "Alumno: Alberto Llamas Gonzalez";

}
