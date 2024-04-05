/******************************************************************************
 * Laboratoire 5
 * GIF-3004 Systèmes embarqués temps réel
 * Hiver 2024
 * Marc-André Gardner
 * 
 * Fichier implémentant les fonctions de l'emulateur de clavier
 ******************************************************************************/

#include "emulateurClavier.h"

FILE* initClavier(){
    // Deja implementee pour vous
    FILE* f = fopen(FICHIER_CLAVIER_VIRTUEL, "wb");
    setbuf(f, NULL);        // On desactive le buffering pour eviter tout delai
    return f;
}


int ecrireCaracteres(FILE* periphClavier, const char* caracteres, size_t len, unsigned int tempsTraitementParPaquetMicroSecondes){
    // TODO ecrivez votre code ici. Voyez les explications dans l'enonce et dans
    // emulateurClavier.h

    enum BufferMode_t current_mode = INIT;
    char buffer[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    char empty_buffer[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    size_t current_byte = 2;
    char current_char;

    for (size_t i = 0; i < len; i++)
    {
        current_char = (char) ascii2UsbHid(caracteres[i]);
        if (current_char < 225)
        {
            if (current_mode == SHIFT)
            {
                fwrite(buffer, LONGUEUR_USB_PAQUET, 1, periphClavier);
                fwrite(empty_buffer, LONGUEUR_USB_PAQUET, 1, periphClavier);
                usleep(tempsTraitementParPaquetMicroSecondes);
                current_byte = 2;
                current_mode = INIT;
                memset(&buffer, 0, 8 * sizeof(char));
            }
            
            current_mode = NO_SHIFT;
            buffer[current_byte] = current_char;
            current_byte++;
        }

        else if (current_char > 225)
        {
            if (current_mode == NO_SHIFT)
            {
                fwrite(buffer, LONGUEUR_USB_PAQUET, 1, periphClavier);
                fwrite(empty_buffer, LONGUEUR_USB_PAQUET, 1, periphClavier);
                usleep(tempsTraitementParPaquetMicroSecondes);
                current_byte = 2;
                current_mode = INIT;
                memset(&buffer, 0, 8 * sizeof(char));
            }

            current_mode = SHIFT;
            buffer[0] = 2;
            current_char -= 225;
            buffer[current_byte] = current_char;
            current_byte++;
        }        

        if (current_byte >= 8)
        {
            fwrite(buffer, LONGUEUR_USB_PAQUET, 1, periphClavier);
            fwrite(empty_buffer, LONGUEUR_USB_PAQUET, 1, periphClavier);
            usleep(tempsTraitementParPaquetMicroSecondes);
            current_byte = 2;
            current_mode = INIT;
            memset(&buffer, 0, 8 * sizeof(char));
        }
    }

    if (current_mode != INIT)
    {
        fwrite(buffer, LONGUEUR_USB_PAQUET, 1, periphClavier);
        fwrite(empty_buffer, LONGUEUR_USB_PAQUET, 1, periphClavier);
        usleep(tempsTraitementParPaquetMicroSecondes);
        current_byte = 2;
        current_mode = INIT;
        memset(&buffer, 0, 8 * sizeof(char));
    }
    
    return 0;
}

unsigned int ascii2UsbHid(char ascii) {
    unsigned int num_value = ascii;

    switch (ascii)
    {
    case '0':
        return 39;
        break;

    case '.':
        return 55;
        break;
    
    case ',':
        return 54;
        break;

    case ' ':
        return 44;
        break;

    case '\n':
        return 40;
        break;

    default:
        if (ascii >= '1' && ascii <= '9') {
            return (num_value - 19);
        }
        else if (ascii >= 'a' && ascii <= 'z') {
            return (num_value - 93);
        }
        else if (ascii >= 'A' && ascii <= 'Z') {
            return (num_value - 61 + 225);
        } 
        break;
    }
    
    return 0;
    
}