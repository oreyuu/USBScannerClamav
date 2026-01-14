#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

/* ============================
   Déclaration des fonctions
   ============================ */
int scan(const char *line);
int formatage(void);
void menu_scan_en_cours(void);
int switchcase(void);
int logs(void);
int ejection(void);

/* ============================
   Variables globales
   ============================ */

// Entiers :
int notification = 0;      // Compteur de notifications
int delay = 3;             // Délai entre deux détections (secondes)
int usb_connecter = 0;     // Flag pour empêcher le lancement multiple du scan
// Chaînes de caractères :
char ok[20];
char ok1[20];
char nomUSB[100];

/* ============================
   Affichage du menu principal
   ============================ */
void menu_scan_en_cours()
{
    system("clear");
    printf("\033[31m"); // Texte en rouge
    printf("╔══════════════════════════════════════╗\n");
    printf("║          USB SCANNER PROGRAM         ║\n");
    printf("╠══════════════════════════════════════╣\n");
    printf("║                                      ║\n");
    printf("║          Scan en cours...            ║\n");
    printf("║                                      ║\n");
    printf("╚══════════════════════════════════════╝\n");
    printf("\033[0m"); // Reset couleur
}

void menu()
{
    system("clear");
    printf("╔═════════════════════════════════════════╗\n");
    printf("║        USB READER MANAGEMENT MENU       ║\n");
    printf("╠═════════════════════════════════════════╣\n");
    printf("║             resultat Scan :             ║\n");
    printf("║                   OK                    ║\n");
    printf("║            resultat Analyse :           ║\n");
    printf("║                   OK                    ║\n");
    printf("╚═════════════════════════════════════════╝\n");

    if (notification < 1)
    {
        printf("\n╔═════════════════════════════════════════╗\n");
        printf("║             Notification LOGS : %d       ║\n", notification);
        printf("╚═════════════════════════════════════════╝\n");
    }
    else
    {
        printf("\033[31m"); // Texte en rouge si notifications > 0
        printf("\n╔═════════════════════════════════════════╗\n");
        printf("║             Notification LOGS : %d       ║\n", notification);
        printf("╚═════════════════════════════════════════╝\n");
        printf("\033[0m"); // Reset couleur
    }
}

/* ============================
   Fonction principale
   ============================ */
int main(void)
{
    menu(); // Affiche le menu principal

    /*
    =====================================================================
    Lance le monitoring udev pour détecter les événements liés aux blocs
    =====================================================================
    */

    FILE *pipe = popen("udevadm monitor --subsystem-match=block --udev", "r"); 

    char line[1024];

    while (fgets(line, sizeof(line), pipe) != NULL) 
    { 
        // Détection d'un ajout d'un périphérique block (ex : USB)
        if (strstr(line, "add") != NULL && strstr(line, "block")) 
        {
            if (!usb_connecter) // Scan possible uniquement si pas déjà connecté
            {
                scan(line);    // Appel de la fonction scan
                usb_connecter = 1; // Marque la clé comme connectée (empêche scans multiples)
            }
        }
        else if (strstr(line, "add") != NULL && strstr(line, "usb") != NULL) 
        {
            usb_connecter = 1; // USB détecté et connecté
        }
        else if (strstr(line, "remove") != NULL && strstr(line, "usb") != NULL) 
        {
            usb_connecter = 0; // Clé débranchée, scan autorisé à nouveau
            system("clear");
            menu();
            printf("Clé USB débranchée.\n");
            sleep(2);
            system("clear");
            menu();
        }
    }

    pclose(pipe);

    return 0;
}

/* ============================
   Fonction de scan automatique de la clé
   ============================ */
int scan(const char *line)
{
    printf("Clé USB détectée\n");

    system("clear");
    menu();


    char *pos_block = strstr(line, "(block)");
    if (!pos_block)
        return -1;

    // Recherche du nom de la partition entre '/' et "(block)"
    const char *start = pos_block;
    while (start > line && *start != '/')
        start--;
    start++;

    size_t len = pos_block - start - 1;
    if (len >= sizeof(nomUSB))
        len = sizeof(nomUSB) - 1;

    strncpy(nomUSB, start, len);
    nomUSB[len] = '\0';

    system("clear");
    menu_scan_en_cours();

    printf("Nom de la partition USB détectée : %s\n", nomUSB);

    char montage[150];
    snprintf(montage, sizeof(montage), "mount /dev/%s /mnt/usb", nomUSB);

    system(montage);


    // Construction de la commande clamscan
    char commande[150];
    snprintf(commande, sizeof(commande), "clamscan -r /mnt/usb");
    

    system(commande);

    // Exécution de la commande et récupération de la sortie
    FILE *fp = popen(commande, "r");
    if (!fp) 
    {
        perror("Erreur lors de l'exécution du scan");
        return -1;
    }

    char buffer[1024];
    int line_count = 0;
    char septieme_ligne[1024] = {0};

    // Lecture ligne par ligne pour extraire la 7ème ligne
    while (fgets(buffer, sizeof(buffer), fp) != NULL) 
    {
        line_count++;
        if (line_count == 7) 
        {
            strncpy(septieme_ligne, buffer, sizeof(septieme_ligne) - 1);
            septieme_ligne[sizeof(septieme_ligne) - 1] = '\0';
        }
    }

    pclose(fp);

    // Nettoyage des caractères de fin (\n, espace, \r) dans la 7ème ligne
    int len_ligne = strlen(septieme_ligne);
    while (len_ligne > 0 && (septieme_ligne[len_ligne - 1] == '\n' || septieme_ligne[len_ligne - 1] == ' ' || septieme_ligne[len_ligne - 1] == '\r'))
        len_ligne--;

    char dernier_char = (len_ligne > 0) ? septieme_ligne[len_ligne - 1] : '\0';

    int virus_count = dernier_char - '0'; // Conversion du caractère en entier

    if (virus_count > 0)
    {
        notification++;
        logs(); // Affiche le menu d'alerte
    }
    else
    {
        system("clear");
        printf("╔════════════════════════════════════════╗\n");
        printf("║                                        ║\n");
        printf("║       ✅ Aucun virus détecté !         ║\n");
        printf("║                                        ║\n");
        printf("╚════════════════════════════════════════╝\n");

        sleep(3);
        system("clear");
        menu();
    }

    return 0;
}

/* ============================
   Menu d'alerte en cas d'infection détectée
   ============================ */
int logs(void)
{
    system("clear");
    printf("╔════════════════════════════════════════════════════╗\n");
    printf("║               ⚠️  ALERTE INFECTION USB ⚠️            ║\n");
    printf("╠════════════════════════════════════════════════════╣\n");
    printf("║ Votre clé USB est infectée par un virus.           ║\n");
    printf("║ Afin de sécuriser vos données, nous avons besoin   ║\n");
    printf("║ de quelques informations vous concernant.          ║\n");
    printf("║                                                    ║\n");
    printf("║ Merci de saisir votre prénom et nom :              ║\n");
    printf("╚════════════════════════════════════════════════════╝\n");

    notification++;

    //variable nom prénom
    char prenom[50];
    char nom[50];


    //inpute pour les variable
    printf("\nVeuillez entré votre prénom :");
    scanf("%s", prenom);
    printf("Veuillez entré votre prénom :");
    scanf("%s", nom);


    char logs[150]; //variable dans le buffer pour la commande des logs
    snprintf(logs, sizeof(logs), "./logs/log.%s.%s", nom, prenom); //création de la commande du fihier de log

    FILE*f = NULL; //création d'un variable "f" pour manpiler les fichier de logs 

    f = fopen(logs, "w");  //ouverture du fichier (ici "log" corresponde a la dernier ligne snprintf)
    if (f == NULL) //si f est nul alors :
    {
        printf("ereure d'ouverture du fichier de log");//message d'ereur
        return 0;
    }

    fprintf(f,"%s::%s", nom, prenom); //ecrir dans le fichier 
    fclose(f); //ferme le fichier
    
    switchcase(); //appelle la fonction switchcase 

    return 0;
}

/* ============================
   Récupération du prénom et nom utilisateur
   ============================ */
int switchcase(void)
{

    int choix = 0;

    system("clear");

    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║               ⚠️ ATTENTION FORMATAGE ⚠️               ║\n");
    printf("╠════════════════════════════════════════════════════════╣\n");
    printf("║  Le formatage va effacer toutes les données            ║\n");
    printf("║  de votre clé USB de façon permanente.                  ║\n");
    printf("║                                                        ║\n");
    printf("║  Voulez-vous vraiment formater la clé USB ?            ║\n");
    printf("║                                                        ║\n");
    printf("║  [1] Oui     [2] Non                                   ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    printf("\nVotre choix : ");

    scanf("%d", &choix);

    while (1)
    {
        switch (choix)
        {
        case 1:
            formatage();
            break;
        
        case 2:
            ejection();
            break;
        
        default:
            printf("Veuillez rentrer une valeur, 1 ou 2 :");
            break;
        }
    }
    

}

/* ============================
   Fonction de formatage (stub)
   ============================ */
int formatage(void)
{
    printf("Formatage de la clé USB en cours...\n");
    // Implémenter la logique de formatage ici
    sleep(3);
    printf("Formatage terminé.\n");
    sleep(2);
    system("clear");
    menu();
    return 0;
}

int ejection(void)
{
    printf("ejection de votre clé");
    sleep(2);
    return 0;
}


