/****************************************************************
	IFT630 - Devoir 2
	Exercice 1 : Résolution de l'équation de Laplace avec la méthode
				 d'itération de Jacobi (cas 2D)

	Étudiants:
		- Pierre-Emmanuel Goffi (18110928)
		- Audrey Lanneville (18068485)

****************************************************************/
#include <iostream>
#include <cmath>
#include <string>
#include "mpi.h"

#define TAG_FROM_MASTER         1
#define TAG_FROM_SLAVE          2

#define MASTER                  0

#define SIZE_M                  6 //Taille de la matrice M
#define LIMITE                 -1 //Valeur limite à atteindre (côtés de M)
#define VAL_MIN                 1 //Valeur de départ minimale (intérieur M)
#define VAL_MAX                 4 //Valeur de départ maximale (intérieur M)

#define EPSILON                 0.01 //Critère d'arrêt #1
#define MAX_ITER                100 //Critère d'arrêt #2

#define NB_LIGNES_INTERNES      (SIZE_M - 2)
#define NB_VALS_INTERNES        ((SIZE_M-2) * (SIZE_M-2)) //Nombre de valeurs à manipuler

double M[SIZE_M][SIZE_M];
double M_copy[SIZE_M][SIZE_M]; //Copie de M pour garder les anciennes valeurs

//Variables utilisées dans la communication
double newVal;
int ligneMin, ligneMax, ligne, colonne;

MPI_Status status;
MPI_Request demande;

/************************************************
  Cette fonction initialise la matrice M permettant
  de résoudre le problème (appelée par le maître)
************************************************/
void initMatrice()
{
	srand(time(0));
	for (int i = 0; i < SIZE_M; i++)
	{
		for (int j = 0; j < SIZE_M; j++)
		{
			bool limite = (i == 0 || j == 0 || i == SIZE_M - 1 || j == SIZE_M - 1);

			M[i][j] = limite ? LIMITE : VAL_MIN + (std::rand() % (VAL_MAX - VAL_MIN + 1));
			M_copy[i][j] = M[i][j];
		}
	}
}

/************************************************
  Cette fonction copie les nouvelles valeurs
  de M à partir de M_copy
************************************************/
void updateMatrice()
{
	for (int i = 1; i < SIZE_M - 1; i++)
	{
		for (int j = 1; j < SIZE_M - 1; j++)
		{
			M[i][j] = M_copy[i][j];
		}
	}
}

/************************************************
  Cette fonction affiche la matrice M
************************************************/
void printMatrice(int iter)
{
	std::cout << std::endl;
	std::cout << "--------- Grille iteration " << iter << " ---------" << std::endl;
	for (int i = 0; i < SIZE_M; i++)
	{
		std::cout << std::endl;
		for (int j = 0; j < SIZE_M; j++)
		{
			double val = floor(M[i][j] * 100.) / 100.;

			std::cout << val << " ";

			//Pour aligner éléments (esthétisme seulement)
			std::string str = std::to_string(val);			
			while (str.at(str.length() - 1) == '0')
			{
				str.pop_back();
			}
			int L = str.length();
			for (int espace = 0; espace < (6 - L); espace++)
			{
				std::cout << " ";
			}
		}
	}
	std::cout << std::endl << std::endl;
}

/************************************************
  Main : Résoud l'équation de Laplace avec la 
  méthode d'itérations de Jacobi
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
		MPI_Abort(MPI_COMM_WORLD, 1);
		exit(EXIT_FAILURE);
	}

	//Gère le nombre de travailleurs selon le nombre de processus (size)
	int nb_slaves = (size - 1 > NB_LIGNES_INTERNES) ? NB_LIGNES_INTERNES : size - 1;

	int lines_per_slave = NB_LIGNES_INTERNES / nb_slaves;
	int reste = NB_LIGNES_INTERNES % nb_slaves;

	int iteration = 1;
	double ecart_moyen = 1;

	bool finished = false;

	while (iteration < MAX_ITER && ecart_moyen > EPSILON)
	{
		double ecart_local = 0.;
		//Initialisation de M par le maître
		if (rank == MASTER)
		{
			if (iteration == 1)
			{
				initMatrice();
				printMatrice(0);
			}

			if (size - 1 > NB_VALS_INTERNES)
			{
				std::cout << "NOTE: Il y a " << size - 1 - NB_VALS_INTERNES << " processus en trop (qui ne font rien).\n" << std::endl;
			}

			//Distribution des lignes à calculer aux esclaves
			for (int i = 1; i <= nb_slaves; i++)
			{
				ligneMin = (i - 1) * lines_per_slave + 1;
				ligneMax = ligneMin + lines_per_slave - 1;

				//Le dernier esclave calcule le reste s'il y a lieu
				if (i == nb_slaves)
				{
					ligneMax += reste;
				}

				MPI_Isend(&ligneMin, 1, MPI_INT, i, TAG_FROM_MASTER, MPI_COMM_WORLD, &demande);
				MPI_Isend(&ligneMax, 1, MPI_INT, i, TAG_FROM_MASTER, MPI_COMM_WORLD, &demande);
			}
		}

		//Envoi à tout le monde le contenu de M
		MPI_Bcast(&M, SIZE_M * SIZE_M, MPI_DOUBLE, 0, MPI_COMM_WORLD);

		//Esclaves
		if (rank != MASTER && rank <= nb_slaves)
		{
			//Attente de réception de la ligne ou des lignes à calculer
			MPI_Recv(&ligneMin, 1, MPI_INT, 0, TAG_FROM_MASTER, MPI_COMM_WORLD, &status);
			MPI_Recv(&ligneMax, 1, MPI_INT, 0, TAG_FROM_MASTER, MPI_COMM_WORLD, &status);

			for (int i = ligneMin; i <= ligneMax; i++)
			{
				for (int j = 1; j < SIZE_M - 1; j++)
				{
					//Calcul nouvelle valeur à la position courante
					double oldVal = M[i][j];
					newVal = (M[i - 1][j] + M[i + 1][j] + M[i][j - 1] + M[i][j + 1]) / 4.;

					ligne = i;
					colonne = j;

					//Envoi du résultat et de la position dans M au maître (non-bloquant)
					MPI_Isend(&newVal, 1, MPI_DOUBLE, 0, TAG_FROM_SLAVE, MPI_COMM_WORLD, &demande);
					MPI_Isend(&ligne, 1, MPI_INT, 0, TAG_FROM_SLAVE, MPI_COMM_WORLD, &demande);
					MPI_Isend(&colonne, 1, MPI_INT, 0, TAG_FROM_SLAVE, MPI_COMM_WORLD, &demande);

					//Somme locale de l'écart au carré entre la nouvelle valeur et l'ancienne
					ecart_local += (oldVal - newVal) * (oldVal - newVal);
				}
			}
		}

		//Calcul de la somme globale à partir de toutes les sommes partielles (locales) et envoi du résultat au maître
		MPI_Reduce(&ecart_local, &ecart_moyen, 1, MPI_DOUBLE, MPI_MAX, MASTER, MPI_COMM_WORLD);

		if (rank == MASTER)
		{
			//Mise à jour de M_copy à partir des résultats des esclaves
			int n = lines_per_slave * (SIZE_M - 2);
			for (int i = 1; i <= nb_slaves; i++)
			{
				n = (i != nb_slaves) ? n : n + (SIZE_M - 2) * reste;
				for (int val = 0; val < n; val++)
				{
					MPI_Recv(&newVal, 1, MPI_DOUBLE, i, TAG_FROM_SLAVE, MPI_COMM_WORLD, &status);
					MPI_Recv(&ligne, 1, MPI_INT, i, TAG_FROM_SLAVE, MPI_COMM_WORLD, &status);
					MPI_Recv(&colonne, 1, MPI_INT, i, TAG_FROM_SLAVE, MPI_COMM_WORLD, &status);

					M_copy[ligne][colonne] = newVal;
				}
			}
			
			//Passage des valeurs de M_copy dans M (maintenant que tous les résultats ont été envoyés)
			updateMatrice();

			//Affichage de la matrice M à l'itération courante
			printMatrice(iteration);

			//Racine carrée de la moyenne des écarts
			ecart_moyen = sqrt(ecart_moyen / NB_VALS_INTERNES);
			std::cout << "Ecart moyen : " << ecart_moyen << std::endl << std::endl;

			iteration++;
		}

		//Envoi à tout le monde des variables servant à évaluer le critère d'arrêt
		MPI_Bcast(&ecart_moyen, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
		MPI_Bcast(&iteration, 1, MPI_INT, 0, MPI_COMM_WORLD);
	}

	if (rank == MASTER)
	{
		std::cout << "FIN DES ITERATIONS : criteres d'arret remplis (iteration >=" << MAX_ITER << " ou ecart moyen < " << EPSILON <<  ")" << std::endl;
	}

	MPI_Finalize();

	return 0;
}