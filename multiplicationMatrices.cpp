/*******************************************************
    IFT630 - Devoir 2
    Exercice 1 : Multiplication de matrices avec MPI

    Étudiants:
        - Pierre-Emmanuel Goffi (18110928)
        - Audrey Lanneville (18068485)

*******************************************************/
#include <iostream>
#include <cmath>
#include "mpi.h"

#define TAG_FROM_ADMIN 1
#define TAG_FROM_WORKER 2

//Tailles des matrices A et B
#define NB_LIGNES_A 2
#define NB_COLONNES_A 3
#define NB_LIGNES_B 3
#define NB_COLONNES_B 2

//Nombre d'éléments dans la matrice résultante à calculer
#define NB_ELEMENTS (NB_LIGNES_A * NB_COLONNES_B)

//Matrices
int A[NB_LIGNES_A][NB_COLONNES_A];
int B[NB_LIGNES_B][NB_COLONNES_B];
int res[NB_LIGNES_A][NB_COLONNES_B];

//Variables qui se passent par messages
int indexMin1D, indexMax1D; //index 1D du range d'éléments qu'un travailleur doit calculer
int ligne, colonne; //index 2D des éléments de la matrice résultante
int resultat; //resultat d'un élément

//Variables pour le Send & Receive
MPI_Status status;
MPI_Request demande;

/************************************************
  Cette fonction initialise les matrices A et B
  et est appelée par l'admin (maître)
************************************************/
void initMatrices()
{
    srand(time(0));
    for (int i = 0; i < NB_LIGNES_A; i++)
    {
        for (int j = 0; j < NB_COLONNES_A; j++)
        {
            A[i][j] = rand() % 10;
        }
    }
    for (int i = 0; i < NB_LIGNES_B; i++)
    {
        for (int j = 0; j < NB_COLONNES_B; j++)
        {
            B[i][j] = rand() % 10;
        }
    }
}

/************************************************
  Cette fonction écrit sur la console les matrices
  A, B et la matrice résultante. 
************************************************/
void printMatrices()
{
    std::cout << std::endl;
    std::cout << "MATRICE A" << std::endl;
    for (int i = 0; i < NB_LIGNES_A; i++)
    {
        std::cout << std::endl;
        for (int j = 0; j < NB_COLONNES_A; j++)
        {
            std::cout << A[i][j] << " ";
        }
    }
    std::cout << std::endl << std::endl;
    std::cout << "MATRICE B" << std::endl;
    for (int i = 0; i < NB_LIGNES_B; i++)
    {
        std::cout << std::endl;
        for (int j = 0; j < NB_COLONNES_B; j++)
        {
            std::cout << B[i][j] << " ";
        }
    }

    std::cout << std::endl << std::endl;
    std::cout << "MATRICE RESULTANTE" << std::endl;
    for (int i = 0; i < NB_LIGNES_A; i++)
    {
        std::cout << std::endl;
        for (int j = 0; j < NB_COLONNES_B; j++)
        {
            std::cout << res[i][j] << " ";
        }
    }
    std::cout << std::endl;
}

/************************************************
  Main : calcule le produit entre A et B
************************************************/
int main(int argc, char* argv[])
{
    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size < 2)
    {
        std::cout << "ERREUR : 2 processus minimum requis (un administrateur et un travailleur)." << std::endl;
        MPI_Finalize();
        exit(EXIT_FAILURE);
    }

    //Gère le nombre de travailleurs selon le nombre de processus (size)
    int nb_travailleurs = (size - 1 > NB_ELEMENTS) ? NB_ELEMENTS : size - 1;

    int nb_elements_par_worker, reste;

    //Administrateur : envoi demandes de calcul
    if (rank == 0)
    {
        initMatrices();

        if (size - 1 > NB_ELEMENTS)
        {
            std::cout << "NOTE: Il y a " << size - 1 - NB_ELEMENTS << " processus en trop (qui ne font rien).\n" << std::endl;
        }

        nb_elements_par_worker = NB_ELEMENTS / nb_travailleurs;
        reste = NB_ELEMENTS % nb_travailleurs;

        //Envoi des demandes de calcul aux travailleurs
        for (int i = 1; i <= nb_travailleurs; i++)
        {
            indexMin1D = (i - 1) * nb_elements_par_worker;
            indexMax1D = indexMin1D + nb_elements_par_worker - 1;

            //Le dernier travailleur calcule le reste s'il y a lieu
            if (i == nb_travailleurs)
            {
                indexMax1D += reste;
            }

            //Envoi des index des éléments à calculer au worker i (non-bloquant)
            MPI_Isend(&indexMin1D, 1, MPI_INT, i, TAG_FROM_ADMIN, MPI_COMM_WORLD, &demande);
            MPI_Isend(&indexMax1D, 1, MPI_INT, i, TAG_FROM_ADMIN, MPI_COMM_WORLD, &demande);
        }
    }

    //Envoi à tout le monde le contenu des matrices A et B
    MPI_Bcast(&A, NB_LIGNES_A * NB_COLONNES_A, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&B, NB_LIGNES_B * NB_COLONNES_B, MPI_INT, 0, MPI_COMM_WORLD);

    //Travailleurs
    if(rank > 0 && rank <= nb_travailleurs)
    {
        //Reçoit les éléments à calculer ainsi que les matrices A et B
        MPI_Recv(&indexMin1D, 1, MPI_INT, 0, TAG_FROM_ADMIN, MPI_COMM_WORLD, &status);
        MPI_Recv(&indexMax1D, 1, MPI_INT, 0, TAG_FROM_ADMIN, MPI_COMM_WORLD, &status);

        for (int k = indexMin1D; k <= indexMax1D; k++)
        {
            //Index 2D de l'élément k dans la matrice résultante
            ligne = k / NB_COLONNES_B;
            colonne = k % NB_COLONNES_B;

            //Calcule de l'élément correspondant
            resultat = 0;
            for (int i = 0; i < NB_COLONNES_A; i++)
            {
                resultat += (A[ligne][i] * B[i][colonne]);
            }

            std::cout << "Processus " << rank << " calcule (" << ligne << ", " << colonne << ") = " << resultat << std::endl;

            //Envoi du résultat de l'élément à l'administrateur (non-bloquant)
            MPI_Isend(&ligne, 1, MPI_INT, 0, TAG_FROM_WORKER, MPI_COMM_WORLD, &demande);
            MPI_Isend(&colonne, 1, MPI_INT, 0, TAG_FROM_WORKER, MPI_COMM_WORLD, &demande);
            MPI_Isend(&resultat, 1, MPI_INT, 0, TAG_FROM_WORKER, MPI_COMM_WORLD, &demande);
        }
    }

    //Administrateur : réception des résultats et écriture dans la matrice résultante
    if (rank == 0)
    {
        int n;
        for (int i = 1; i <= nb_travailleurs; i++)
        {
            n = (i == nb_travailleurs) ? nb_elements_par_worker + reste : nb_elements_par_worker;
            for (int element = 0; element < n; element++)
            {
                MPI_Recv(&ligne, 1, MPI_INT, i, TAG_FROM_WORKER, MPI_COMM_WORLD, &status);
                MPI_Recv(&colonne, 1, MPI_INT, i, TAG_FROM_WORKER, MPI_COMM_WORLD, &status);
                MPI_Recv(&resultat, 1, MPI_INT, i, TAG_FROM_WORKER, MPI_COMM_WORLD, &status);

                res[ligne][colonne] = resultat;
            }
        }

        printMatrices();
    }

    MPI_Finalize();

    return 0;
}