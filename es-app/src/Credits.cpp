#include "Credits.h"

#include <iostream>
#include <fstream>
#include "SystemData.h"

int getCredit()
{
    std::ifstream fichier(SystemData::getCreditsFile()); // Ouvre le fichier en lecture
    // Vérifie si le fichier a pu être ouvert
    if (!fichier.is_open())
    {
        std::cerr << "Le fichier n'existe pas ou n'a pas pu être ouvert." << std::endl;
        return 0; // Retourne 0 si le fichier n'existe pas
    }
    int nombre;
    fichier >> nombre; // Lit le nombre depuis le fichier
    // Vérifie si la lecture a réussi
    if (fichier.fail())
    {
        std::cerr << "Erreur lors de la lecture du fichier." << std::endl;
        fichier.close();
        return 0; // Retourne 0 si la lecture échoue (fichier vide ou mauvais format)
    }
    fichier.close(); // Ferme le fichier après la lecture
    return nombre;   // Retourne le nombre lu
}
void incrementCredit()
{
    int nombre = getCredit();                                                             // Lire le nombre actuel
    nombre++;                                                                             // Incrémenter le nombre
    std::ofstream fichier(SystemData::getCreditsFile(), std::ios::out | std::ios::trunc); // Ouvre le fichier en écriture (crée le fichier si nécessaire)
    if (!fichier.is_open())
    {
        std::cerr << "Impossible d'ouvrir ou de créer le fichier." << std::endl;
        return;
    }
    fichier << nombre; // Écrit le nombre incrémenté dans le fichier
    fichier.close();   // Ferme le fichier après l'écriture
    std::cout << "Le nombre a été incrémenté et enregistré dans " << SystemData::getCreditsFile() << std::endl;
}
void decrementCredit(int countCredit)
{
    int nombre = getCredit(); // Lire le nombre actuel
    // Décrémenter seulement si le nombre est supérieur à 0
    if (nombre > 0)
    {
        nombre -= countCredit;
    }
    // Ouvre le fichier en écriture, et s'il n'existe pas, il sera créé
    std::ofstream fichier(SystemData::getCreditsFile(), std::ios::out | std::ios::trunc);
    if (!fichier.is_open())
    {
        std::cerr << "Impossible d'ouvrir ou de créer le fichier." << std::endl;
        return;
    }
    fichier << nombre; // Écrit le nombre décrémenté dans le fichier
    fichier.close();   // Ferme le fichier après l'écriture
    std::cout << "Le nombre a été décrémenté et enregistré dans " << SystemData::getCreditsFile() << std::endl;
}