/*
	Tiago Rafael Cardoso Santos,2021229679
	Tomás Cunha da Silva,2021214124
*/

#include "structs.h"

int main(int argc, char* argv[]) {
    // Verifica se recebe os 7 parâmetros
    if (argc != 7) {
        fprintf(stderr," mobile_user {plafond inicial} {número máximo de pedidos de autorização} {intervalo VIDEO} {intervalo MUSIC} {intervalo SOCIAL} {dados a reservar}");
        return 1;
    }

    int inputs[6]; // Array para armazenar dados do input

    for (int i = 0; i < 6; i++) {
        inputs[i] = atoi(argv[i + 1]); // Convertendo argumentos para inteiros
        if (inputs[i] <= 0) {
            printf("ARGUMENTS ARE NOR INTEGERS OR POSITIVE\n");
            return 1;
        }
    }

    mobile(inputs,getpid());

    return 0;
}
