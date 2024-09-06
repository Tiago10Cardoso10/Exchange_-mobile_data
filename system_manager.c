/*
	Tiago Rafael Cardoso Santos,2021229679
	Tomás Cunha da Silva,2021214124
*/

#include "structs.h"

int main(int argc, char* argv[]) {
    // Verifica se o número de argumentos é válido
    
    if (argc != 2) {
        fprintf(stderr, "5g_auth_platform {config-file}\n");
        return 0;
    }
	
    pid_system_manager = getpid();
    

    system_manager(argv[1],pid_system_manager);

    return 0;	
}
