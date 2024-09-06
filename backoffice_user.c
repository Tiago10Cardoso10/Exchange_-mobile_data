/*
	Tiago Rafael Cardoso Santos,2021229679
	Tomás Cunha da Silva,2021214124
*/

#include "structs.h" // Certifique-se de incluir as definições necessárias

int main(int argc, char* argv[]) {
    // Verificar se recebeu algum argumento
    if (argc != 1) {
        fprintf(stderr, "backoffice_user\n");
        return 1;
    }

    // Inicializar o processo do backoffice user
    back_office(1);

    return 0;
}
