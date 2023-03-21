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

// Ejecucion: g++ -std=c++11 -pthread -Wfatal-errors -o peaje peaje.cpp scd.cpp

#include <iostream>
#include <iomanip>
#include <cassert>
#include <random>
#include <thread>
#include "scd.h"

using namespace std;
using namespace scd;

const unsigned int n_coches = 8;

int coches_cabina1 = 0;
int coches_cabina2 = 0;

class Peaje : public HoareMonitor {

private:
    CondVar c_cabina1, c_cabina2;
public:
    Peaje();
    int llegada_peaje();
    void pagado(int cab);
};

Peaje::Peaje() {
    c_cabina1 = newCondVar();
    c_cabina2 = newCondVar();
}

int Peaje::llegada_peaje() {
    int x;
    if (coches_cabina2 >= coches_cabina1) {
        x = 1;
        coches_cabina1++;
        cout << "[CAB 1] Llega coche nuevo" << endl << flush;
        if (coches_cabina1 > 1) {
            cout << "[CAB 1] Coche esperando a que paguen" << endl;
            c_cabina1.wait();
        }
    } else {
        x = 2;
        coches_cabina2++;
        cout << "[CAB 2] Llega coche nuevo" << endl << flush;
        if (coches_cabina2 > 1) {
            cout << "[CAB 2] Coche esperando a que paguen" << endl;
            c_cabina2.wait();
        }
    }

    return x;
}

void Peaje::pagado(int cab) {
    if (cab == 1) {
        coches_cabina1--;
        cout << "\t[CAB 1] Coche pagado, siguiente coche." << endl;
        c_cabina1.signal();
    } 

    if (cab == 2) {
        coches_cabina2--;
        cout << "\t[CAB 2] Coche pagado, siguiente coche." << endl;
        c_cabina2.signal();
    }
}

void funcion_hebra_coche(MRef<Peaje> monitor, int i) {
    int cab;
    while (true) {
        //espera llegada peaje
        this_thread::sleep_for(chrono::milliseconds(aleatorio<500, 1000>()));
        cout << "Coche " << i << " ha llegado..." << endl;
        cab = monitor->llegada_peaje();
        this_thread::sleep_for(chrono::milliseconds(aleatorio<500, 1000>()));
        monitor->pagado(cab);
        cout << "Coche " << i << " ha pagado y se retira..." << endl;

    }
}

int main(int argc, char const *argv[]) {
    MRef<Peaje> monitor = Create<Peaje>();
    thread hebras_coches[n_coches];

    for (int i = 0; i < n_coches; i++)
        hebras_coches[i] = thread (funcion_hebra_coche, monitor, i);

    for (int i = 0; i < n_coches; i++)
        hebras_coches[i].join();
}