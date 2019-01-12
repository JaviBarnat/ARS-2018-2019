//Practica tema 5, Barrientos Gonzalez Javier

//Definimos las librerias que vamos a necesitar

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

#define MAXL 500 //Definimos la constante de tamaño de buffer

int main(int argc, char *argv[]){

	//Definimos los valores iniciales para el servidor

	int puerto = 0; //Variable donde vamos a guardar el puerto del servidor
	struct servent *serverPort; //Variable para guardar el valor del puerto por defecto de QOTD

	switch(argc){
		case 1: //El servidor se inicia con el puerto de QOTD por defecto de manera automatica
			serverPort = getservbyname("qotd", "udp");
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
			break;
	}

	//Creamos el socket propio del servidor que sera el que enlace con la peticion del cliente
	int sock;
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock < 0){
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
	errBind = bind(sock, (struct sockaddr *) &servidor, sizeof(servidor));
	if(errBind == -1){
		//Si el bind no ha sido creado correctamente devuelve un mensaje de error y finaliza la ejecucion
		perror("bind()");
		exit(EXIT_FAILURE);
	}

	//Creamos una estructura cliente para guardar los datos de este cuando envie una peticion y poder
	//usar esos datos para enviar de nuevo la respuesta
	struct sockaddr_in cliente;
	socklen_t longitud_cliente = sizeof(cliente);

	//Bucle infinito para mantener el servidor escuchando todo el rato
	while(true){

		//Definimos los valores de los buffers que vamos a usar, tanto para enviar como para obtener la cita del fichero
		char buffer[500];
		static char buffQuote[MAXL];

		//Recibimos por el socket creado con anterioridad el mensaje desde el cliente y guardamos su informacion en la estructura presente mas arriba
		int rec = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *) &cliente, &longitud_cliente);
		if(rec == -1){
			//Si presenta un error a la hora de recibir el mensaje, devuelve un mensaje de error y finaliza la ejecucion
			perror("recvfrom()");
			exit(EXIT_FAILURE);
		}

		//Borramos el contenido de ambos buffers para evitar problemas a la hora de guardar los nuevos datos en estos
		memset(buffer, '\0', sizeof(buffer));
		memset(buffQuote, '\0', sizeof(buffQuote));

		//Obtenemos la cita del dia a traves del bucle ofrecido por el profesor en el cual creamos un fichero con la cita a enviar
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

		//Escribimos y juntamos los mensajes en el buffer de llegada (vaciado anteriormente) que usaremos a la vez para guardar
		//el contenido del mensaje a enviar
		char *m = "Quote Of The Day from vm2520:\n";
		strcat(buffer, m);
		strcat(buffer, buffQuote);
		int len_mensaje = strlen(buffer)+1; //El tamanyo tiene que ser +1 porque strlen no lee el caracter final de cadena

		//Preparamos el envio al cliente con el mensaje en el buffer, junto a su longitud y a los datos
		//del cliente presente en la estrcutura creada anteriormente
		int errSend;
		errSend = sendto(sock, buffer, len_mensaje, 0, (struct sockaddr *) &cliente, sizeof(cliente));
		if(errSend == -1){
			//Si presenta un error a la hora de enviar un mensaje muestra el error y detiene la ejecucion
			perror("sendto()");
			exit(EXIT_FAILURE);
		}
	}
	//Finaliza el programa. Al tener bucle infinito no acabará nunca
	return 0;
}


