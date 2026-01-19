#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

/* ============================
   Déclaration des fonctions
   ============================ */
int scan(const char *line);
int formatage(void);
void menu_scan_en_cours(void);
void menu(void);
int switchcase(int nb_virus);
int logs(char *nom, char *prenom, int nb_virus, int a_ete_nettoye);
int ejection_securite(void); // Renommé pour éviter les conflits

/* ============================
   Variables globales
   ============================ */
int notification = 0;
int usb_connecter = 0;
char nomUSB[100];

// --- Menu Principal ---
void menu() {
    system("clear");
    printf("╔═════════════════════════════════════════╗\n");
    printf("║        USB READER MANAGEMENT MENU       ║\n");
    printf("╠═════════════════════════════════════════╣\n");
    printf("║        Veuillez brancher une clé        ║\n");
    printf("║                    USB                  ║\n");
    printf("╚═════════════════════════════════════════╝\n");
    
    if (notification > 0) {
        printf("\033[1;31m╔══════════════════════════════════════════╗\n");
        printf("║  📩 LOGS D'INFECTION EN ATTENTE : %-7d║\n", notification);
        printf("╚══════════════════════════════════════════╝\033[0m\n");
    } else {
        printf("\033[1;34m╔══════════════════════════════════════════╗\n");
        printf("║  📩 AUCUNE NOTIFICATION DE LOG           ║\n");
        printf("╚══════════════════════════════════════════╝\033[0m\n");
    }
}

void menu_scan_en_cours() {
    system("clear");
    printf("\033[1;33m╔══════════════════════════════════════╗\n");
    printf("║          USB SCANNER PROGRAM         ║\n");
    printf("╠══════════════════════════════════════╣\n");
    printf("║          Scan en cours...            ║\n");
    printf("╚══════════════════════════════════════╝\033[0m\n");
}

int main(void) 
{
    // Création des répertoires nécessaires
    system("mkdir -p /mnt/usb ./log");
    //afichage menu 
    menu();

    //chercher les evenements udev pour les cle usb branchées
    FILE *pipe = popen("udevadm monitor --subsystem-match=block --udev", "r"); 
    if (!pipe) return 1;


    //regarde et vérifi les evenements
    char line[1024];
    while (fgets(line, sizeof(line), pipe) != NULL) { 
        if (strstr(line, "add") != NULL && strstr(line, "block") != NULL) {
            if (!usb_connecter) {
                scan(line);
                usb_connecter = 1;
            }
        } else if (strstr(line, "remove") != NULL) {
            usb_connecter = 0;
            menu();
        }
    }
    pclose(pipe);
    return 0;
}

int scan(const char *line) 
{

    //prend le nom de la cle usb

    char *pos_block = strstr(line, "(block)");
    if (!pos_block) return -1;
    const char *start = pos_block;
    while (start > line && *start != '/') start--;
    start++;
    size_t len = pos_block - start - 1;
    strncpy(nomUSB, start, (len < sizeof(nomUSB)) ? len : sizeof(nomUSB)-1);
    nomUSB[len] = '\0';

    //affichage menu scan en cours
    menu_scan_en_cours();
    char cmd_mount[150];
    snprintf(cmd_mount, sizeof(cmd_mount), "mount /dev/%s1 /mnt/usb 2>/dev/null", nomUSB);
    system(cmd_mount);

    //lance le scan clamscan
    char commande[150];
    snprintf(commande, sizeof(commande), "clamscan -r /mnt/usb");
    FILE *fp = popen(commande, "r");
    char buffer[1024];
    int virus_found = 0;
    while (fgets(buffer, sizeof(buffer), fp) != NULL) {
        if (strstr(buffer, "Infected files: ")) virus_found = atoi(strstr(buffer, ":") + 1);
    }
    pclose(fp);


    //affichage resultat scan

    if (virus_found > 0) {
        notification++;
        switchcase(virus_found);
    } else {
        printf("\033[1;32m╔══════════════════════════════════════════════════════╗\n");
        printf("║               ✅ ANALYSE TERMINÉE ✅                 ║\n");
        printf("╠══════════════════════════════════════════════════════╣\n");
        printf("║      Aucun virus détecté sur la clé : %-14s ║\n", nomUSB);
        printf("╚══════════════════════════════════════════════════════╝\033[0m\n");
        sleep(3);
        menu();
    }
    return 0;
}

int switchcase(int nb_virus) 
{

    // Demande d'action à l'utilisateur
    char prenom[50], nom[50];
    int choix_utilisateur = 0;

    system("clear");
    printf("\033[1;31m╔══════════════════════════════════════════════════════╗\n");
    printf("║              🚨 ALERTE : INFECTION 🚨                ║\n");
    printf("╠══════════════════════════════════════════════════════╣\n");
    printf("║  Menace(s) sur cette clé : %-25d ║\n", nb_virus);
    printf("║                                                      ║\n");
    printf("║  Veuillez décliner votre identité pour le rapport :  ║\n");
    printf("╚══════════════════════════════════════════════════════╝\033[0m\n");
    
    //entrer le nom et prenom dans le buffer
    printf(" > Prénom : "); scanf("%49s", prenom);
    printf(" > Nom    : "); scanf("%49s", nom);
    
    printf("\n╔══════════════════════════════════════════════════════╗\n");
    printf("║                💾 ACTION REQUISE 💾                  ║\n");
    printf("╠══════════════════════════════════════════════════════╣\n");
    printf("║  [1] OUI : Formater (Supprime TOUT)                  ║\n");
    printf("║  [2] NON : Éjecter la clé infectée                   ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n");
    printf("Choix : ");
    scanf("%d", &choix_utilisateur);
    
    // Enregistre le log et effectue l'action choisie
    if (choix_utilisateur == 1) {
        logs(nom, prenom, nb_virus, 1); // 1 = Nettoyé
        formatage();
    } else {
        logs(nom, prenom, nb_virus, 0); // 0 = Non nettoyé
        ejection_securite();
    }
    return 0;
}

int logs(char *nom, char *prenom, int nb_virus, int a_ete_nettoye) 
{

    // Enregistre les logs dans un fichier
    char path[200];
    snprintf(path, sizeof(path), "./log/log.%s.%s.txt", nom, prenom);
    FILE *f = fopen(path, "a");
    if (f == NULL) return -1;


    // Obtient la date et l'heure actuelles
    time_t maintenant;
    time(&maintenant);
    struct tm *t = localtime(&maintenant);
    char date_formatee[64];
    strftime(date_formatee, sizeof(date_formatee), "%d/%m/%Y %H:%M:%S", t);

    fprintf(f, "============================================\n");
    fprintf(f, " DATE DE DÉTECTION : %s\n", date_formatee);
    fprintf(f, "============================================\n");
    fprintf(f, " Utilisateur : %s %s\n", prenom, nom);
    fprintf(f, " Périphérique: /dev/%s1\n", nomUSB);
    fprintf(f, " Menaces     : %d\n", nb_virus);
    fprintf(f, " Action prise: %s\n", (a_ete_nettoye == 1) ? "FORMATAGE (CLE NETTOYEE)" : "EJECTION (DANGER PERSISTANT)");
    fprintf(f, "--------------------------------------------\n\n");

    fclose(f);
    return 0;
}

int formatage(void) 
{

    // Formate la clé USB
    printf("\033[33mNettoyage et formatage de /dev/%s1...\033[0m\n", nomUSB);
    char cmd[150];
    snprintf(cmd, sizeof(cmd), "umount /mnt/usb 2>/dev/null && mkfs.vfat /dev/%s1", nomUSB);
    system(cmd);
    //
    printf("✅ Formatage terminé. La clé est maintenant saine.\n");
    sleep(3);
    menu();
    return 0;
}

int ejection_securite(void) 
{
    // Éjecte la clé USB sans la formater
    system("umount /mnt/usb 2>/dev/null");
    printf("\033[1;31m⚠️  CLÉ ÉJECTÉE SANS NETTOYAGE. RISQUE PERSISTANT.\033[0m\n");
    sleep(4);
    menu();
    return 0;
}