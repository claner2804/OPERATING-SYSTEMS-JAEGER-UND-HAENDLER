#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <signal.h>

// Definiere Konstanten für die Kapazität der Brieftasche, die Zeit zur Münzauffüllung, die Simulationszeit und die Anzahl der Jäger
#define WALLET_CAPACITY 10
#define COIN_REPLENISH_TIME 2 // Sekunden
#define SIMULATION_TIME 300   // 5 Minuten in Sekunden
#define HUNTERS 5

// Mutex-Sperre zur Synchronisation des Zugriffs auf die Brieftasche
pthread_mutex_t wallet_lock;
int coins = WALLET_CAPACITY; // Aktuelle Anzahl der Münzen in der Brieftasche
bool running = true; // Flag, das angibt, ob die Simulation läuft

// Signal-Handler Funktion für SIGINT
void handle_sigint(int sig) {
    printf("\n[Signal] SIGINT empfangen, beende die Simulation...\n");
    running = false; // Setze das Flag auf false, um alle Threads zu beenden
}

// Thread-Funktion zum Auffüllen der Münzen in der Brieftasche
void* replenish_coin(void* arg) {
    while (running) {
        sleep(COIN_REPLENISH_TIME); // Warte die festgelegte Auffüllzeit, bevor eine Münze hinzugefügt wird
        pthread_mutex_lock(&wallet_lock); // Sperre die Brieftasche, um sicheren Zugriff zu gewährleisten
        if (coins < WALLET_CAPACITY) {
            coins++; // Füge eine Münze hinzu, wenn Platz vorhanden ist
            printf("[Replenisher] Eine Münze wurde hinzugefügt. Aktuelle Münzen: %d\n", coins);
        }
        pthread_mutex_unlock(&wallet_lock); // Entsperre die Brieftasche nach der Änderung
    }
    return NULL;
}

// Thread-Funktion für jeden Jäger, um Münzen zu sammeln
void* hunter_thread(void* arg) {
    int hunter_id = *((int*)arg); // Hole die ID des Jägers aus dem Argument
    int collected_coins = 0; // Verfolge die Anzahl der gesammelten Münzen dieses Jägers
    free(arg); // Befreie den für die Jäger-ID zugewiesenen Speicher

    while (running) {
        bool collected = false;
        pthread_mutex_lock(&wallet_lock); // Sperre die Brieftasche, um Münzen zu prüfen und zu sammeln
        if (coins > 0) {
            coins--; // Nimm eine Münze aus der Brieftasche
            collected = true;
            printf("[Jäger %d] Eine Münze gesammelt. Verbleibende Münzen: %d\n", hunter_id, coins);
        }
        pthread_mutex_unlock(&wallet_lock); // Entsperre die Brieftasche nach der Überprüfung
        if (collected) {
            collected_coins++; // Erhöhe die Anzahl der von diesem Jäger gesammelten Münzen
        }
        usleep((rand() % 400 + 100) * 1000); // Zufällige Verzögerung zwischen 0,1s und 0,5s, um die Aktion des Jägers zu simulieren
    }

    printf("Jäger %d hat %d Münzen gesammelt.\n", hunter_id, collected_coins); // Drucke die Gesamtanzahl der von diesem Jäger gesammelten Münzen
    return NULL;
}

int main() {
    srand(time(NULL)); // Initialisiere den Zufallszahlengenerator mit der aktuellen Zeit
    pthread_t hunters[HUNTERS]; // Array der Jäger-Threads
    pthread_t replenisher; // Thread zum Auffüllen der Münzen
    pthread_mutex_init(&wallet_lock, NULL); // Initialisiere die Mutex-Sperre

    // Registriere den Signal-Handler für SIGINT
    signal(SIGINT, handle_sigint);

    printf("[Main] Starte den Auffüll-Thread und die Jäger-Threads.\n");

    // Starte den Auffüll-Thread, um regelmäßig Münzen zur Brieftasche hinzuzufügen
    pthread_create(&replenisher, NULL, replenish_coin, NULL);

    // Starte die Jäger-Threads, um Münzen zu sammeln
    for (int i = 0; i < HUNTERS; i++) {
        int* hunter_id = malloc(sizeof(int)); // Speicherplatz für die Jäger-ID reservieren
        if (hunter_id == NULL) { // Überprüfen, ob die Speicherzuweisung erfolgreich war
            perror("Fehler bei der Speicherzuweisung");
            exit(EXIT_FAILURE);
        }
        *hunter_id = i; // Setze die Jäger-ID
        pthread_create(&hunters[i], NULL, hunter_thread, hunter_id); // Erstelle den Jäger-Thread
        printf("[Main] Jäger %d Thread gestartet.\n", i);
    }

    // Simulationszeit oder bis SIGINT empfangen wird
    sleep(SIMULATION_TIME); // Führe die Simulation für die definierte Dauer (5 Minuten) aus
    running = false; // Beende die Simulation

    printf("[Main] Beende die Simulation und warte auf alle Threads.\n");

    // Warte, bis alle Jäger-Threads beendet sind
    for (int i = 0; i < HUNTERS; i++) {
        pthread_join(hunters[i], NULL); // Warte auf das Ende jedes Jäger-Threads
        printf("[Main] Jäger %d Thread beendet.\n", i);
    }

    pthread_join(replenisher, NULL); // Warte auf das Ende des Auffüll-Threads
    printf("[Main] Auffüll-Thread beendet.\n");

    pthread_mutex_destroy(&wallet_lock); // Zerstöre die Mutex-Sperre, um Ressourcen freizugeben
    printf("[Main] Programm beendet.\n");

    return 0;
}