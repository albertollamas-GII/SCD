// -----------------------------------------------------------------------------
//
// Sistemas concurrentes y Distribuidos.
// Seminario 2. Introducción a los monitores en C++11.
//
// Archivo: examen 
//
// Ejemplo de un monitor en C++11 con semántica SU, para el problema
// del productor/consumidor, con productor y consumidor únicos.
// Opcion LIFO
//
// Historial:
// Creado el 30 Sept de 2022. (adaptado de prodcons2_su.cpp)
// 20 oct 22 --> paso este archivo de FIFO a LIFO, para que se corresponda con lo que dicen las transparencias
// -----------------------------------------------------------------------------------

// Ejecucion: g++ -std=c++11 -pthread -Wfatal-errors -o prod_cons_ex prod_cons_ex.cpp scd.cpp

#include <iostream>
#include <iomanip>
#include <cassert>
#include <random>
#include <thread>
#include "scd.h"

using namespace std;
using namespace scd;

constexpr int
    num_items = 30; // número de items a producir/consumir
int
    siguiente_dato = 0; // siguiente valor a devolver en 'producir_dato'

constexpr int
    min_ms = 5,  // tiempo minimo de espera en sleep_for
    max_ms = 20; // tiempo máximo de espera en sleep_for

const int num_productores = 3, num_consumidores = 3; // numero de productores y consumidores (divisores de num_items)

// Productor produce num_items/num_productores
// Consumidor consume num_items/num_consumidores
unsigned items_producidos[num_productores] = {0},
         items_consumidos[num_consumidores] = {0};

mutex
    mtx; // mutex de escritura en pantalla
unsigned
    cont_prod[num_items] = {0}, // contadores de verificación: producidos
    cont_cons[num_items] = {0}; // contadores de verificación: consumidos

//**********************************************************************
// funciones comunes a las dos soluciones (fifo y lifo)
//----------------------------------------------------------------------

int producir_dato(int num_hebra)
{
    static int contador = 0;
    items_producidos[num_hebra]++;
    this_thread::sleep_for(chrono::milliseconds(aleatorio<min_ms, max_ms>()));
    mtx.lock();
    cout << "producido: " << contador << " (hebra " << num_hebra << ", total: "
         << items_producidos[num_hebra] << ")" << endl
         << flush;
    mtx.unlock();
    cont_prod[contador]++;
    return contador++;
}
//----------------------------------------------------------------------

void consumir_dato(unsigned valor_consumir, int num_hebra)
{
    if (num_items <= valor_consumir)
    {
        cout << " dato == " << valor_consumir << ", num_items == " << num_items << endl;
        assert(valor_consumir < num_items);
    }
    cont_cons[valor_consumir]++;
    items_consumidos[num_hebra]++;
    this_thread::sleep_for(chrono::milliseconds(aleatorio<20, 100>()));
    mtx.lock();
    cout << "                  consumido: " << valor_consumir << " (hebra " << num_hebra
         << ", total: " << items_consumidos[num_hebra] << ")" << endl;
    mtx.unlock();
}
//----------------------------------------------------------------------

void test_contadores()
{
    bool ok = true;
    cout << "comprobando contadores ...." << endl;

    for (unsigned i = 0; i < num_items; i++)
    {
        if (cont_prod[i] != 1)
        {
            cout << "error: valor " << i << " producido " << cont_prod[i] << " veces." << endl;
            ok = false;
        }
        if (cont_cons[i] != 1)
        {
            cout << "error: valor " << i << " consumido " << cont_cons[i] << " veces" << endl;
            ok = false;
        }
    }
    if (ok)
        cout << endl
             << flush << "solución (aparentemente) correcta." << endl
             << flush;
}

void ini_contadores()
{
    for (unsigned i = 0; i < num_items; i++)
    {
        cont_cons[i] = 0;
        cont_prod[i] = 0;
    }
}

// *****************************************************************************
// clase para monitor buffer, version FIFO, semántica SC, multiples prod/cons

//---------------------------SOLUCION LIFO--------------------------------------
class ProdConsSU_LIFO : public HoareMonitor
{
private:
    static const int            // constantes ('static' ya que no dependen de la instancia)
        num_celdas_total = 10,  //   núm. de entradas del buffer
        num_celdas_pares = 5,   // buffer v2 (hacer para que se pasen como parametros)
        num_celdas_impares = 4; // buffer v1
    int
        v1[num_celdas_impares],
        primera_libre_impares;
    int
        v2[num_celdas_pares],
        primera_libre_pares;
    CondVar       // colas condicion:
        ocupadas, //  cola donde espera el consumidor (n>0)
        libres_pares,
        libres_impares; //  cola donde espera el productor  (n<num_celdas_total)

public:                // constructor y métodos públicos
    ProdConsSU_LIFO(); // constructor
    int leer();        // extraer un valor (sentencia L) (consumidor)
    void escribir_pares(int valor);
    void escribir_impares(int valor); // insertar un valor (sentencia E) (productor)
};
// -----------------------------------------------------------------------------

ProdConsSU_LIFO::ProdConsSU_LIFO()
{
    primera_libre_pares = primera_libre_impares = 0;
    ocupadas = newCondVar();
    libres_pares = newCondVar();
    libres_impares = newCondVar();
}
// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

int ProdConsSU_LIFO::leer()
{
    // esperar bloqueado hasta que 0 < primera_libre
    if (primera_libre_pares == 0 && primera_libre_impares == 0)
        ocupadas.wait();

    int valor = 0;

    //evaluo primera libre
    if (0 < primera_libre_impares && primera_libre_pares > 0)
    {
        //elijo un vector para escribir de forma aleatoria
        int lectura = aleatorio<1,2>();
        if (lectura == 2) {
            primera_libre_pares--;
            valor = v2[primera_libre_pares];
            libres_pares.signal();
        } else {
            primera_libre_impares--;
            valor = v1[primera_libre_impares];
            libres_impares.signal();
        }
    } else if (primera_libre_impares > 0) {
        primera_libre_impares--;
        valor = v1[primera_libre_impares];
        libres_impares.signal();
    } else {
        primera_libre_pares--;
        valor = v2[primera_libre_pares];
        libres_pares.signal();
    }

    // devolver valor
    return valor;
}
// -----------------------------------------------------------------------------

void ProdConsSU_LIFO::escribir_pares(int valor)
{
    // esperar bloqueado hasta que primera_libre < num_celdas_total
    if (primera_libre_pares == num_celdas_pares)
        libres_pares.wait();

    assert(primera_libre_pares < num_celdas_pares);

    // hacer la operación de inserción, actualizando estado del monitor
    v2[primera_libre_pares] = valor;
    primera_libre_pares++;

    // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
    ocupadas.signal();
}

void ProdConsSU_LIFO::escribir_impares(int valor)
{
    // esperar bloqueado hasta que primera_libre < num_celdas_total
    if (primera_libre_impares == num_celdas_impares)
        libres_pares.wait();

    assert(primera_libre_impares < num_celdas_impares);

    // hacer la operación de inserción, actualizando estado del monitor
    v1[primera_libre_impares] = valor;
    primera_libre_impares++;

    // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
    ocupadas.signal();
}
// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora_LIFO(MRef<ProdConsSU_LIFO> monitor, int num_hebra)
{
    for (unsigned i = 0; i < num_items; i += num_productores)
    {
        int valor = producir_dato(num_hebra);
        if (num_hebra % 2 == 0)
            monitor->escribir_pares(valor);
        else 
            monitor->escribir_impares(valor);
    }
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora_LIFO(MRef<ProdConsSU_LIFO> monitor, int num_hebra)
{
    for (unsigned i = num_hebra; i < num_items; i += num_consumidores)
    {
        int valor = monitor->leer();
        consumir_dato(valor, num_hebra);
    }
}
// -----------------------------------------------------------------------------

//******************************************************************************
//-------------------------------SOLUCION FIFO----------------------------------
class ProdConsSU_FIFO : public HoareMonitor
{
private:
    static const int              // constantes ('static' ya que no dependen de la instancia)
        num_celdas_total = 10;    //   núm. de entradas del buffer
    int                           // variables permanentes
        buffer[num_celdas_total], //   buffer de tamaño fijo, con los datos
        primera_libre,            //   indice de celda de la próxima inserción
        primera_ocupada,          //    indice de celda de proxima extracción
        num_celdas_ocupadas;      //   numero de celdas ocupadas variable condición

    CondVar       // colas condicion:
        ocupadas, //  cola donde espera el consumidor (n>0)
        libres;   //  cola donde espera el productor  (n<num_celdas_total)

public:                       // constructor y métodos públicos
    ProdConsSU_FIFO();        // constructor
    int leer();               // extraer un valor (sentencia L) (consumidor)
    void escribir(int valor); // insertar un valor (sentencia E) (productor)
};
// -----------------------------------------------------------------------------

ProdConsSU_FIFO::ProdConsSU_FIFO()
{
    primera_libre = 0;
    primera_ocupada = 0;
    num_celdas_ocupadas = 0;
    ocupadas = newCondVar();
    libres = newCondVar();
}
// -----------------------------------------------------------------------------
// función llamada por el consumidor para extraer un dato

int ProdConsSU_FIFO::leer()
{
    // esperar bloqueado hasta que 0 < num_celdas_ocupadas
    while (num_celdas_ocupadas == 0)
        ocupadas.wait();

    // cout << "leer: ocup == " << primera_libre << ", total == " << num_celdas_total << endl ;
    assert(0 < num_celdas_ocupadas);

    // hacer la operación de lectura, actualizando estado del monitor
    const int valor = buffer[primera_ocupada];
    primera_ocupada = (primera_ocupada + 1) % num_celdas_total;
    num_celdas_ocupadas--;

    // señalar al productor que hay un hueco libre, por si está esperando
    libres.signal();

    // devolver valor
    return valor;
}
// -----------------------------------------------------------------------------

void ProdConsSU_FIFO::escribir(int valor)
{
    // esperar bloqueado hasta que primera_libre < num_celdas_total
    if (num_celdas_ocupadas == num_celdas_total)
        libres.wait();

    // cout << "escribir: ocup == " << primera_libre << ", total == " << num_celdas_total << endl ;
    assert(num_celdas_ocupadas < num_celdas_total);

    // hacer la operación de inserción, actualizando estado del monitor
    buffer[primera_libre] = valor;
    primera_libre = (primera_libre + 1) % num_celdas_total;
    num_celdas_ocupadas++;

    // señalar al consumidor que ya hay una celda ocupada (por si esta esperando)
    ocupadas.signal();
}
// *****************************************************************************
// funciones de hebras

void funcion_hebra_productora_FIFO(MRef<ProdConsSU_FIFO> monitor, int num_hebra)
{
    for (unsigned i = num_hebra; i < num_items; i += num_productores)
    {
        int valor = producir_dato(num_hebra);
        monitor->escribir(valor);
    }
}
// -----------------------------------------------------------------------------

void funcion_hebra_consumidora_FIFO(MRef<ProdConsSU_FIFO> monitor, int num_hebra)
{
    for (unsigned i = num_hebra; i < num_items; i += num_consumidores)
    {
        int valor = monitor->leer();
        consumir_dato(valor, num_hebra);
    }
}

/**
 * @brief Ejecuta el programa dependiendo de la versión del problema
 * @param type: tipo de problema a ejecutar
 */
int ejecutar(int tipo)
{
    if (tipo != 0 && tipo != 1)
    {
        cout << "Error: no ha indicado un número válido";
        return 1;
    }

    thread hebra_consumidora[num_consumidores];
    thread hebra_productora[num_productores];

    if (tipo == 0)
    {
        MRef<ProdConsSU_LIFO> monitor = Create<ProdConsSU_LIFO>();
        cout << "Ejecutando solución LIFO..." << endl;
        for (int i = 0; i < num_consumidores; i++)
            hebra_consumidora[i] = thread(funcion_hebra_consumidora_LIFO, monitor, i);
        for (int i = 0; i < num_productores; i++)
            hebra_productora[i] = thread(funcion_hebra_productora_LIFO, monitor, i);
    }
    else
    {
        MRef<ProdConsSU_FIFO> monitor = Create<ProdConsSU_FIFO>();
        cout << "Ejecutando solución FIFO..." << endl;
        for (int i = 0; i < num_consumidores; i++)
            hebra_consumidora[i] = thread(funcion_hebra_consumidora_FIFO, monitor, i);
        for (int i = 0; i < num_productores; i++)
            hebra_productora[i] = thread(funcion_hebra_productora_FIFO, monitor, i);
    }

    for (int i = 0; i < num_consumidores; i++)
        hebra_consumidora[i].join();
    for (int i = 0; i < num_productores; i++)
        hebra_productora[i].join();

    test_contadores();

    return 0;
}

int main()
{
    cout << "¿Qué versión del problema desea ejecutar?" << endl
         << "   0 : versión LIFO" << endl
         << "   1 : versión FIFO" << endl
         << "Inserte la versión aquí -> ";
    int version_problema;
    cin >> version_problema;

    return ejecutar(version_problema);
}
