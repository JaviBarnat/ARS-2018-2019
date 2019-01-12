// Practica tema 6, Barrientos Gonzalez Javier

#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h> //Para poder usar errno y perror()
#include <stdio.h> //Para poder usar perror()
#include <stdlib.h> //Para poder usar EXIT_FAILURE
#include <netinet/ip.h> //Para definir sockaddr_in
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h> //Libreria para el uso de strings
#include <stdbool.h> //Libreria para el uso de true o false
#include <signal.h>

#define MAXL 500 //Definimos la constante de tamaño de buffer

//Definimos los valores para los sockets que se creen
int sockCliente, sockServidor;

//Funcion encargada de cerrar los puertos abiertos en la maquina cuando reciban la senyal ctrlc
void signalHandler(int signal){
	if(signal == SIGINT){
		if(shutdown(sockServidor, 2) < 0){
			//Comprobamos que el socket se cierra correctamente
			perror("shutdown()");
			exit(EXIT_FAILURE);
		}
		exit(0);
	}
}

int main(int argc, char *argv[]){

        //Definimos los valores iniciales para el servidor
        int puerto = 0; //Variable donde vamos a guardar el puerto del servidor
        struct servent *serverPort; //Variable para guardar el valor del puerto por defecto de QOTD
	signal(SIGINT, signalHandler); //Asignamos los valores de la funcion para cerrar los puertos con ctrl+c

        switch(argc){
                case 1: //El servidor se inicia con el puerto de QOTD por defecto de manera automatica
                        serverPort = getservbyname("qotd", "tcp");
                        puerto = serverPort->s_port;
                        break;

		case 3: //El servidor se inicia en el puerto asignado por el usuario
                        if(strcmp(argv[1], "-p") != 0){
                                //Comprobamos si la opcion insertada es -p para seleccion del puerto, si no, cancelamos la ejecucion
                                printf("La opcion no es valida. Inserte una opcion valida.\n");
                                exit(EXIT_FAILURE);
                        }
                        //Si la opcion es correcta obtenemos el puerto del arguemnto y po pasamos
                        //a network byte order con htons
                        sscanf(argv[2], "%d", &puerto);
                        puerto = htons(puerto);
                        break;

                default: //Error y mensaje para aquellos casos no validos
                        printf("La composicion del comando no es correcta.\n./qot-server [-p server-port].");
			exit(EXIT_FAILURE);
                        break;
        }

	//Creamos el socket propio del servidor que sera el que enlace con la peticion del cliente
        sockServidor = socket(AF_INET, SOCK_STREAM, 0);
        if(sockServidor < 0){
                //Si el socket no se ha creado correctamente devuelve un mensaje de error y finaliza la ejecucion
                perror("socket()");
                exit(EXIT_FAILURE);
        }

        //Creamos una estructura para el servidor y sus valores
        //En esta estructura definimos el valor del puerto, si es por defecto el 17 o si es el que pasa el
        //usuario como argumento. La direccion IP es la de por defecto
        struct sockaddr_in servidor;
        servidor.sin_family = AF_INET;
        servidor.sin_port = puerto;
        servidor.sin_addr.s_addr = INADDR_ANY;

        //Creamos el bind referido a la estructura anterior del servidor y al socket inicial creado
        int errBind;
        errBind = bind(sockServidor, (struct sockaddr *) &servidor, sizeof(servidor));
        if(errBind == -1){
                //Si el bind no ha sido creado correctamente devuelve un mensaje de error y finaliza la ejecucion
                perror("bind()");
                exit(EXIT_FAILURE);
        }

        //Creamos una estructura cliente para guardar los datos de este cuando envie una peticion y poder
        //usar esos datos para enviar de nuevo la respuesta
        struct sockaddr_in cliente;
        socklen_t longitud_cliente = sizeof(cliente);

	//Estructura listen para escuchar el socket asignado
	int escucha;
	escucha = listen(sockServidor, 5);
	if(escucha < 0){
		//Si no es posible escuchar en el socket, devuelve el error por pantalla y finaliza la ejecucion
		perror("listen()");
		exit(EXIT_FAILURE);
	}


        //Bucle infinito para mantener el servidor escuchando todo el rato
	while(true){

		//Aceptamos una conxion entrante en el servidor
		sockCliente = accept(sockServidor, (struct sockaddr *) &cliente, &longitud_cliente);
		if(sockCliente < 0){
			//Si se produce un error en la aceptacion de la conexion mostramos el error y paramos la ejecucion
			perror("accept");
			exit(EXIT_FAILURE);
		}

		//Obtenemos el valor del proceso que anda ejecutando el bucle
		//Si es un hijo, realiza la tarea, sino, continua el bucle otra vez
		int hijo = fork();
		if(hijo == 0){

			//Creamos la estructura del buffer para almacenar el mensaje y lo borramos por si presenta contenido anterior
	                static char buffQuote[MAXL];
			char buffer[150];
        	        memset(buffQuote, '\0', sizeof(buffQuote));
        	        memset(buffer, '\0', sizeof(buffer));

			//Almacenamos la primera frase de la cadena en el buffer de envio
                	char *m = "Quote Of The Day from vm2520:\n";
	                strcat(buffer, m);

	                //Obtenemos la cita del dia a traves del bucle ofrecido por el profesor en el cual creamos un fichero con la $
	                //el cual leemos en bucle hasta el final de fichero y guardando en un buffer su contenido caracter a caracter
	                system("/usr/games/fortune -s > /tmp/tt.txt");
	                FILE *fichero = fopen("/tmp/tt.txt", "r");
	                int n = 0; //Contador
	                char bb; //Variable donde vamos a guardar el caracter leido de fichero en bucle
	                do{
	                        bb = fgetc(fichero);
	                        if(bb == EOF){ //Si el caracter leido es un fin de fichero
	                                buffQuote[n++] = 0; //AInsertamos un cero referido a final de cadenay salimos del bucle
	                                break;
	                        }
	                        buffQuote[n++] = bb;
	                }while(n < MAXL-1);
	                fclose(fichero); //Cerramos el fichero
	                n = 0; //Reseteamos el contador

			strcat(buffer, buffQuote);
        	        int len_mensaje = strlen(buffer)+1; //El tamanyo tiene que ser +1 porque strlen no lee el caracter final de cadena

			//Enviamos el mensaje almacenado en el buffer creado con anterioridad con los datos del cliente
			int envio;
			envio = send(sockCliente, buffer, len_mensaje, 0);
			if(envio < 0){
				//Si se ha producido un fallo, este se muestra por pantalla y se finaliza la ejecicion
				perror("send()");
				exit(EXIT_FAILURE);
			}

			//Paramos de enviar y recibir mensajes desde el socket cliente
			close(sockCliente); //Cerramos el socket
			exit(0);

		}else{
			//Si es el padre no hace nada
		}
	}
	//Finalizacion del programa
	return 0;
}
