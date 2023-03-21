// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Práctica 3. Implementación de algoritmos distribuidos con MPI
//
// Archivo: filosofos-plantilla.cpp
// Implementación del problema de los filósofos (sin camarero).
// Plantilla para completar.
//
// Historial:
// Actualizado a C++11 en Septiembre de 2017
// -----------------------------------------------------------------------------


#include <mpi.h>
#include <thread> // this_thread::sleep_for
#include <random> // dispositivos, generadores y distribuciones aleatorias
#include <chrono> // duraciones (duration), unidades de tiempo
#include <iostream>

using namespace std;
using namespace std::this_thread ;
using namespace std::chrono ;

#define ETIQ_SENTARSE 1
#define ETIQ_LEVANTARSE 2

const int
   num_filosofos = 5 ,              // número de filósofos 
   num_fc  = 2*num_filosofos,       // número de filósofos y camareros 
   num_procesos  = num_fc + 1 ,     // número de procesos total 
   id_camarero   = 2*num_filosofos;

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

void funcion_filosofos( int id )
{
  int id_ten_izq = (id+1)              % num_fc, //id. tenedor izq.
      id_ten_der = (id+num_fc-1) % num_fc, //id. tenedor der.
      peticion;

  while ( true )
  {
    cout <<"Filósofo " <<id << " solicita sentarse en la mesa" <<endl;
    MPI_Ssend(&peticion, 1, MPI_INT, id_camarero, ETIQ_SENTARSE, MPI_COMM_WORLD);    
    
    cout <<"Filósofo " <<id << " solicita ten. izq." <<id_ten_izq <<endl;
    // ... solicitar tenedor izquierdo (completar)
    MPI_Ssend(&peticion, 1, MPI_INT, id_ten_izq, 0, MPI_COMM_WORLD);

    cout <<"Filósofo " <<id <<" solicita ten. der." <<id_ten_der <<endl;
    // ... solicitar tenedor derecho (completar)
    MPI_Ssend(&peticion, 1, MPI_INT, id_ten_der, 0, MPI_COMM_WORLD);

    cout <<"Filósofo " <<id <<" comienza a comer" <<endl ;
    sleep_for( milliseconds( aleatorio<10,100>() ) );

    cout <<"Filósofo " <<id <<" suelta ten. izq. " <<id_ten_izq <<endl;
    // ... soltar el tenedor izquierdo (completar)
    MPI_Ssend(&peticion, 1, MPI_INT, id_ten_izq, 0, MPI_COMM_WORLD);
    cout<< "Filósofo " <<id <<" suelta ten. der. " <<id_ten_der <<endl;
    // ... soltar el tenedor derecho (completar)
    MPI_Ssend(&peticion, 1, MPI_INT, id_ten_der, 0, MPI_COMM_WORLD);
    cout <<"Filósofo " <<id << " solicita levantarse de la mesa" <<endl;
    MPI_Ssend(&peticion, 1, MPI_INT, id_camarero, ETIQ_LEVANTARSE, MPI_COMM_WORLD);    cout <<"Filósofo " <<id << " solicita ten. izq." <<id_ten_izq <<endl;

    cout << "Filosofo " << id << " comienza a pensar" << endl;
    sleep_for( milliseconds( aleatorio<10,100>() ) );
 }
}
// ---------------------------------------------------------------------

void funcion_tenedores( int id )
{
  int valor, id_filosofo ;  // valor recibido, identificador del filósofo
  MPI_Status estado ;       // metadatos de las dos recepciones

  while ( true )
  {
     // ...... recibir petición de cualquier filósofo (completar)
     MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &estado);
     // ...... guardar en 'id_filosofo' el id. del emisor (completar)
     id_filosofo= estado.MPI_SOURCE;
     cout <<"Ten. " <<id <<" ha sido cogido por filo. " <<id_filosofo <<endl;

     // ...... recibir liberación de filósofo 'id_filosofo' (completar)
     MPI_Recv(&valor, 1, MPI_INT, id_filosofo, 0, MPI_COMM_WORLD, &estado);

     cout <<"Ten. "<< id << " ha sido liberado por filo. " <<id_filosofo <<endl ;
  }
}
// ---------------------------------------------------------------------
void funcion_camarero() {
    int valor, id_filosofo, num_sentados = 0, ETIQ_aceptable;
    MPI_Status estado;

    while (true) {
        // 1. determinar si atiende a peticiones de sentarse y levantarse o sólo de levantarse
        if ( num_sentados < num_filosofos - 1 ) 
            ETIQ_aceptable = MPI_ANY_TAG;
        else
            ETIQ_aceptable = ETIQ_LEVANTARSE;
        
        // 2. recibir un mensaje del emisor o emisores aceptables
        MPI_Recv(&valor, 1, MPI_INT, MPI_ANY_SOURCE, ETIQ_aceptable, MPI_COMM_WORLD, &estado);
        id_filosofo = estado.MPI_SOURCE;

        // 3. procesar mensaje recibido
        switch (estado.MPI_TAG) {
            case ETIQ_SENTARSE:
                cout << "Filósofo " << id_filosofo << " se sienta en la mesa" << endl;
                num_sentados++;
                break;
            case ETIQ_LEVANTARSE:
                cout << "Filósofo " << id_filosofo << " se levanta de la mesa" << endl;
                num_sentados--;
                break;
        }

        cout << "Tenemos sentados " << num_sentados << " filósofos" << endl;
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
      if (id_propio == id_camarero)
         funcion_camarero();
      else {
         // ejecutar la función correspondiente a 'id_propio'
         if ( id_propio % 2 == 0 )          // si es par
            funcion_filosofos( id_propio ); //   es un filósofo
         else                               // si es impar
            funcion_tenedores( id_propio ); //   es un tenedor
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

// ---------------------------------------------------------------------
/*
Descripción de la solución al problema:

Primero, en la función de los filósofos, hemos incluido las acciones de sentarse y levantarse:


// Solicitar sentarse en la mesa al camarero
MPI_Ssend(NULL, 0, MPI_INT, id_camarero, etiq_sentar, MPI_COMM_WORLD);

// Confirmación de que el filósofo se puede sentar
MPI_Recv(NULL, 0, MPI_INT, id_camarero, etiq_sentar, MPI_COMM_WORLD, &estado);

...

// Levantarse de la mesa
MPI_Ssend(NULL, 0, MPI_INT, id_camarero, etiq_levantar, MPI_COMM_WORLD);

...

Como vemos, se han utilizado etiquetas para distinguir entre las operaciones. También, en la
operación para sentarse, el filósofo tiene que hacer un receive para esperar la confirmación
del camarero para hacerlo, ya que si hay 4 filósofos sentados tiene que esperar a que uno
se levante. Para levantarse esto no hace falta, se puede hacer sin la confirmación del camarero.



Segundo, en la función camarero, se ha implementado la funcionalidad descrita en el guión de
la siguiente manera (muy parecida al problema prodcons2-mu):

Se trata de un bucle infinito en el que primero determinamos el estado de la mesa y recibimos
los mensajes con etiquetas apropiadas según dicho estado:

if ( s < 4 ) // Si hay menos de 4 filósofos, se puede sentar
   id_etiq_aceptable = MPI_ANY_TAG ; // Para sentarse o levantarse
// Si no, solo se puede levantar
else 
   id_etiq_aceptable = etiq_levantar;

MPI_Recv( NULL, 0, MPI_INT, MPI_ANY_SOURCE, id_etiq_aceptable, MPI_COMM_WORLD, &estado);
id_filosofo = estado.MPI_SOURCE;


El último paso es decidir qué hacer en función del mensaje recibido, esto es, una petición
para sentarse o levantarse:

switch( estado.MPI_TAG ) { // leer emisor del mensaje en metadatos
      // petición para sentarse
      case etiq_sentar:
        s++; // Incrementamos el número de filósofos sentados
        // Le decimos al filósofo que se siente
        MPI_Ssend( NULL, 0, MPI_INT, id_filosofo, etiq_sentar, MPI_COMM_WORLD);
        break;
      // petición para levantarse
      case etiq_levantar:
        s--; // decrementamos el número de filósofos sentados
        break;
}

Y, obviamente, en el main se ha asignado al proceso camarero la función correspondiente:

// ejecutar la función correspondiente a 'id_propio'
if (id_propio == id_camarero)
	funcion_camarero();
else{
	if ( id_propio % 2 == 0 )  // si es par
   		funcion_filosofos( id_propio ); // es un filósofo
	else  // si es impar
   		funcion_tenedores( id_propio ); // es un tenedor
}

*/