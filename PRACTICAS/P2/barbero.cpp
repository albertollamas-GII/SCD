/*
2 tipo de hebras concurrentes:
    * Barbero (1)
    * Clientes (N)
En la barberia hay:
    * 1 silla para cortar el pelo
    * Una sala de espera para los clientes, con una cantidad de
    sillas al menos igual al número de clientes (N)
g++ -std=c++11 -pthread -Wfatal-errors -o barbero barbero.cpp scd.cpp
*/

#include <iostream>
#include <iomanip>
#include <cassert>
#include <random>
#include <thread>
#include "scd.h"

using namespace std;
using namespace scd;

const unsigned int N = 8;

void retardo() {
    chrono::milliseconds duracion_retardo( aleatorio<10,100>() );
    this_thread::sleep_for(duracion_retardo);
}
class Barberia : public HoareMonitor
{
private:
    /**
     * @brief Cola en la que duerme el barbero
     */
    CondVar barbero;

    /**
     * @brief Cola en la que esperan las hebras cliente si la silla está
     * ocupada
     */
    CondVar salaEspera;

    /**
     * @brief Cola que representa si hay alguien pelándose
     */
    CondVar silla;

    /**
     * @brief Número de clientes que están en la barbería
     */
    int num_clientes;

public:
    Barberia();
    void cortarPelo(int cliente);
    void siguienteCliente();
    void finCliente();
};

Barberia::Barberia()
{
    barbero = newCondVar();
    salaEspera = newCondVar();
    silla = newCondVar();
    num_clientes = 0;
}

void Barberia::cortarPelo(int cliente)
{
    num_clientes++;
    cout << "El cliente " << cliente << " entra a que le corten el pelo" << endl;
    // si el barbero está durmiendo
    if (!barbero.empty())
    {
        cout << "El cliente " << cliente << " despierta al barbero" << endl;
        barbero.signal();
    }

    if (num_clientes > 1)
    { // esperamos si hay alguien pelándose o en cola
        cout << "El cliente " << cliente << " acaba de entrar en la sala de espera." << endl;
        salaEspera.wait();
    }

    cout << "El cliente " << cliente << " entra a pelarse" << endl;
    silla.wait(); // esperamos a que el cliente se pele
    cout << "El cliente " << cliente << " se ha pelado y se marcha" << endl;
}

void Barberia::siguienteCliente() {
    if ( num_clientes == 0 ) {
        cout << "------------El barbero está durmiendo" << endl;
        barbero.wait();
    }

    if ( silla.empty() ) {  // no se está pelando a nadie
        cout << "------------El barbero llama a un cliente" << endl;
        salaEspera.signal();
    }
}

void Barberia::finCliente() {
    cout << "------------------El barbero ha terminado de pelar al cliente" << endl;
    silla.signal();
    num_clientes--;
}

void hebra_barbero(MRef<Barberia> monitor) {
    while (true) {
        monitor->siguienteCliente();    // espera a un cliente
        retardo();
        monitor->finCliente();          // avisa al cliente de que le ha cortado el pelo
    }
}

void hebra_clientes(MRef<Barberia> monitor, int num_cliente) {
    while (true) {
        monitor->cortarPelo(num_cliente);   // cliente solicita que le corten el pelo
        retardo();            // al cliente le crece el pelo (retraso)
    }
}

int main() {
    cout << "___________________________________________________" << endl
         << endl << "PROBLEMA DEL BARBERO | MONITOR SU" << endl
         << "___________________________________________________" << endl << endl;

    MRef<Barberia> monitor = Create<Barberia>();

    thread hebra_b(hebra_barbero, monitor);
    thread hebra_c[N];
    for ( int i = 0; i < N; i++ )
        hebra_c[i] = thread(hebra_clientes, monitor, i);
    
    hebra_b.join();
    for ( int i = 0; i < N; i++ ) hebra_c[i].join();

    return 0;
}