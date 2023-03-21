//**********************************************************************
// Compilación y ejecución
// mpicxx -std=c++11 -o food_truck food_truck.cpp
// mpirun --oversubscribe -np 14 ./food_truck
//**********************************************************************

#include <mpi.h>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <iostream>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;


const int
    num_alumnos = 12 ,    //(van del 0 al 11)
    id_dependiente = num_alumnos,   // id 12
    id_cocinero = id_dependiente + 1,   //id 13
    etiq_1_bocata   = 1,
    etiq_2_bocatas  = 2,
    num_procesos    = num_alumnos + 2;

//----------------------------------------------------------------------

template< int min, int max > int aleatorio()
{
  static default_random_engine generador( (random_device())() );
  static uniform_int_distribution<int> distribucion_uniforme( min, max ) ;
  return distribucion_uniforme( generador );
}

// ---------------------------------------------------------------------

void proceso_alumno( int num_alumno )
{
  int   var,                      // valor recibido
        etiqueta_alumno;
  MPI_Status estado ;       // metadatos de las dos recepciones

  while ( true )
  { 
    if ( num_alumno % 2 == 0 ){          // si es par, es un alumno "NORMAL"
        etiqueta_alumno = etiq_1_bocata;
    }
    else{                               // si es impar, es un alumno "TRAGÓN"
        etiqueta_alumno = etiq_2_bocatas;
    }

    //retardo aleatorio
    sleep_for( milliseconds( aleatorio<10,100>()) );
    cout << "(alumno" << num_alumno << ") Camina hacia el camión "<< endl << flush;

    //enviar petición
    MPI_Ssend( &var, 1, MPI_INT, id_dependiente, etiqueta_alumno, MPI_COMM_WORLD );

    //recibir bocata
    MPI_Recv ( &var, 1, MPI_INT, id_dependiente, etiqueta_alumno, MPI_COMM_WORLD, &estado );

    //retardo aleatorio
    sleep_for( milliseconds( aleatorio<10,100>()) );
    cout << "(alumno" << num_alumno << ") Come mucha grasa "<< endl << flush;
  }
}

// ---------------------------------------------------------------------

void proceso_cocinero( )
{
  int    var ;                 // valor recibido
  MPI_Status estado ;       // metadatos de las dos recepciones

  while ( true )
  { 
    //recibir aviso para preparar bocatas
    MPI_Recv ( &var, 1, MPI_INT, id_dependiente, 0, MPI_COMM_WORLD, &estado );

    //retardo aleatorio
    sleep_for( milliseconds( aleatorio<10,100>()) );
    cout << "COCINERO: Preparar 20 bocatas en el camión "<< endl << flush;

    //enviar aviso bocatas preparados
    MPI_Ssend( &var, 1, MPI_INT, id_dependiente, 0, MPI_COMM_WORLD );
  }
}

// ---------------------------------------------------------------------

void proceso_dependiente( )
{
  int   var,                          // valor recibido
        bocatas_camion  = 20,
        id_emisor,
        tag_emisor;
  MPI_Status estado ;                   // metadatos de las dos recepciones

  while ( true )
  { 
    if (bocatas_camion >=3){            //si quedan en la tienda 3 bocatas o más
        //recibir de cualquier alumno
        MPI_Recv ( &var, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &estado );
    }else if(bocatas_camion >= 1){      //si queda al menos un bocata
        MPI_Recv ( &var, 1, MPI_INT, MPI_ANY_SOURCE, etiq_1_bocata, MPI_COMM_WORLD, &estado );
    }else{                              //si no quedan bocatas
        MPI_Ssend( &var, 1, MPI_INT, id_cocinero, 0, MPI_COMM_WORLD );
        //recibir mensaje de cocinero
        MPI_Recv ( &var, 1, MPI_INT, id_cocinero, 0, MPI_COMM_WORLD, &estado );
    }

    // procesar el mensaje recibido
    id_emisor = estado.MPI_SOURCE;
      
    if(id_emisor != id_cocinero){    //si el emisor es un alumno
        tag_emisor =  estado.MPI_TAG;
        if (tag_emisor == etiq_1_bocata){
            bocatas_camion --;
            cout << "(-1) Quedan " << bocatas_camion << " bocatas en el camión" << endl ;
            MPI_Ssend( &var, 1, MPI_INT, id_emisor, etiq_1_bocata, MPI_COMM_WORLD );
        }else if (tag_emisor == etiq_2_bocatas){
            bocatas_camion -= 2;
            cout << "(-2) Quedan " << bocatas_camion << " bocatas en el camión" << endl ;
            MPI_Ssend( &var, 1, MPI_INT, id_emisor, etiq_2_bocatas, MPI_COMM_WORLD );
        }

    }else{      //si ha sido el cocinero
        bocatas_camion += 20;
        cout << "¡ El camion tiene 20 nuevos bocatas! --> " << bocatas_camion << endl ;
      }

  }
}

// ---------------------------------------------------------------------

int main( int argc, char** argv )
{
   int id_propio, num_procesos_actual ;

   MPI_Init( &argc, &argv );
   MPI_Comm_rank( MPI_COMM_WORLD, &id_propio );
   MPI_Comm_size( MPI_COMM_WORLD, &num_procesos_actual );


   if ( num_procesos == num_procesos_actual )
   {
      // ejecutar la función correspondiente a 'id_propio'
    if (id_propio == id_dependiente){
        proceso_dependiente();
    } else if (id_propio == id_cocinero){
        proceso_cocinero();
    }else{
        proceso_alumno( id_propio ); 
    }

   }
   else
   {
      if ( id_propio == 0 ) // solo el primero escribe error, indep. del rol
      { cout << "el número de procesos esperados es:    " << num_procesos << endl
             << "el número de procesos en ejecución es: " << num_procesos_actual << endl
             << "(programa abortado)" << endl ;
      }
   }

   MPI_Finalize( );
   return 0;
}


/*

Un food-truck está aparcado cerca de la ETSIIT y sólo vende bocatas de panceta con
morcilla untada. Es frecuentado por 12 alumnos de SCD poco cuidadosos con su
alimentación (procesos 0 al 11). 

Los alumnos con identificador par compran un bocata en
cada iteración y los alumnos con identificador impar son más “tragones” y compran dos
bocatas en cada iteración. 

En cada iteración, los alumnos mandan un mensaje al proceso
dependiente (rank 12) con etiquetas distintas según el número de bocatas a comprar. Si
hay menos de 3 bocatas en el camión, el proceso dependiente solo acepta peticiones de
un bocata.

 Cuando no quedan bocatas, el proceso dependiente manda un mensaje al
proceso cocinero (rank 13), que se encarga de preparar en el camión 20 nuevos bocatas.

Suponer que el camión tiene inicialmente 20 bocatas, considerar todos los procesos como
bucles infinitos e implementar dicho programa en MPI con operaciones de paso de
mensaje síncronas y mensajes para seguir la traza del programa (el archivo se deberá
llamar “ejerciciop2.cpp”).

*/