// Practica tema 8, Barrientos Gonzalez Javier

#include "ip-icmp-ping.h" //Fichero con las estructuras de datos necesarias
#include <stdio.h>
#include <string.h> //Libreria necesaria para el tratamiento de cadenas
#include <stdlib.h>

int main(int argc, char* argv[]){

	//Definimos los valores de base
	int bandera = 0;
	struct in_addr addr;

	//Comprobamos con un switch las condiciones que nos pasan como parametros
	switch(argc){

		//Case 2: solo recibimos el nombre del programa y la direccion ip
		//Si esta no es correcta, paramos el programa
		case 2:
			if(inet_aton(argv[1], &addr) == 0){
				printf("Direccion IP no valida. Pruebe con una direccion valida.\n");
				exit(EXIT_FAILURE);
			}
			break;

		//Case 3: recibimos el nombre del programa, la direccion ip destino y la opcion extra
		//Si la direccion ip y/o la opcion no son correctas se para el programa
		case 3:
			if(inet_aton(argv[1], &addr) == 0){
				printf("Direccion IP no valida. Pruebe con una direccion valida.\n");
				exit(EXIT_FAILURE);
			}

			if(strcmp(argv[2], "-v") != 0){
				printf("Opcion no valida. Inserte una opcion valida.\n");
				exit(EXIT_FAILURE);
			}

			bandera = 1;
			break;

		//Si no se cumple ninguna de las otras condiciones entonces mostramos el mensaje de error y finalizamos el programa
		default:
			printf("La composicion no es correcta: miping direccion-ip [-v]\n");
			exit(EXIT_FAILURE);
			break;
	}

	//Creamos el socket con la configuracion correcta
	int sock;
	sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if(sock < 0){
		perror("socket()");
		exit(EXIT_FAILURE);
	}

	//Estructura para el destino del paquete ICMP
	struct sockaddr_in ping;
	ping.sin_family = AF_INET;
	ping.sin_port = 0; //El puerto es secundario porque solo llega a la capa TCP/IP
	ping.sin_addr = addr;
	socklen_t pingL = sizeof(ping);


	//Creacion del datagrama ICMP para su envio
	ECHORequest echoRequest;

	//Si tenemos activada la opcion -v mostramos el mensaje de creacion de la cabecera
	if(bandera == 1){
		printf("-> Generando cabecera ICMP\n");
	}

	//Configuracion del datagrama con el contenido por defecto
	echoRequest.icmpHeader.Type = 8;
	echoRequest.icmpHeader.Code = 0;
	echoRequest.ID = getpid();
	echoRequest.SeqNumber = 0;
	strcpy(echoRequest.payload, "PAYLOAD"); //Rellenamos el payload con texto aleatorio para su funcionamiento

	//Si tenemos activada la opcion -v mostramos los detalles de la crecion de la cabecera ICMP
	if(bandera == 1){
		printf("-> Type: %u\n", echoRequest.icmpHeader.Type);
		printf("-> Code: %u\n", echoRequest.icmpHeader.Code);
		printf("-> Identifier (pid): %u\n", echoRequest.ID);
		printf("-> Seq. number: %u\n", echoRequest.SeqNumber);
		printf("-> Cadena a enviar: %s\n", echoRequest.payload);
	}

	//Calculamos el checksum que contendra el datagrama ICMP
	//Para ello seguimos las instrucciones presents en los apuntes

	echoRequest.icmpHeader.Checksum = 0; //Inicializamos los bytes del checksum a cero

	int numShorts; //Variable entera que es el tamaanyo del datagrama dividido entre dos
	numShorts = sizeof(echoRequest)/2;

	unsigned short int *puntero; //Puntero para recorrer los elementos de 16 bits del datagrama

	unsigned int acumulador = 0; //Variable para ir acumulando los resultados parciales

	//Inicializamos el puntero al inicio del datagrama ICMP y lo vamos recorriendo
	puntero = (unsigned short int *) &echoRequest;

	//Recorremos en bucle
	int i;
	for(i = 0; i < numShorts; i++){
		acumulador = acumulador + (unsigned int) *puntero;
		puntero++;
	}

	//Una vez tenemos la suma de todos los elementos de 16 bits tenemos que sumar la aprte
	//alta del acuulador con la parte baja
	//Hay que hacerlo 2 veces
	//Para finalizar hacemos un not al resultado para obtener su complemento a uno

	acumulador = (acumulador >> 16) + (acumulador & 0x0000ffff);
	acumulador = (acumulador >> 16) + (acumulador & 0x0000ffff);
	acumulador = ~acumulador;

	//Guardamos el acumulador en la variable del checksum del datagrama
	echoRequest.icmpHeader.Checksum = (unsigned short int) acumulador;

	//Si tenemos activa la opcion -v mostramos los detalles del checksum y su tamaño total
	if(bandera == 1){
		printf("-> Checksum: %x\n", echoRequest.icmpHeader.Checksum);
		printf("-> Tamaño total de paquete ICMP %lu\n", sizeof(echoRequest));
	}

	//Enviamos el datagrama a la direccion de destino
	int envio;
	envio = sendto(sock, &echoRequest, sizeof(echoRequest), 0, (struct sockaddr *) &ping, sizeof(ping));
	if(envio < 0){
		//Si no se ha enviado correctamente mostramos el mensaje de error y finalizamos la ejecucion
		perror("sendto()");
		exit(EXIT_FAILURE);
	}

	//Imprimimos el mensaje de envio del paquete ICMP
	printf("Paquete ICMP enviado a %s\n", argv[1]);

	//Recibimos la respuesta
	ECHOResponse  echoResponse;
	int llegada;
	llegada = recvfrom(sock, &echoResponse, 516, 0, (struct sockaddr *) &ping, &pingL);
	if(llegada < 0){
		//Si no se ha recibido correctamente mostramos el mensaje de error y finalizamos la ejecucion
		perror("recvfrom()");
		exit(EXIT_FAILURE);
	}

	//Imprimimos la llegada correcta de la respuesta desde la direccion ip seleccionada
	printf("Respuesta recibida desde %s\n", argv[1]);

	//Dependiendo de los valores presetes en el campo Type de la respuesta tenemos que mostrar un
	//resultado u otro
	//A su vez, dentro de ellos, dependiendo del codigo es una respuesta u otra
	//Para ello realizaremos switches para seleccionar la respuesta dependiendo de los campos del paquete

	switch(echoResponse.icmpHeader.Type){

		case 0:
			//Si tenemos activada la opcion -v mostramos los detalles del paquete recibido
			if(bandera == 1){
				printf("-> Tamaño de la respuest %lu\n", sizeof(echoResponse));
				printf("-> Cadena recibida: %s\n", echoResponse.payload);
				printf("-> Identifier (pid): %u\n", echoResponse.ID);
				printf("-> TTL: %u\n", echoResponse.ipHeader.TTL);
			}

			//La respuesta ha llegado correctamente
			printf("Descripcion de la respuesta: Echo reply (Type 0, Code 0)\n");
			break;

		case 3:

			//El destino no ha sido alcanzado
			printf("Destination Unreachable\n");

			//Existen varios casos
			switch(echoResponse.icmpHeader.Code){

				case 0:
					printf("Destination network unreachable (Type 3, Code 0)\n");
					break;

				case 1:
                                        printf("Destination host unreachable (Type 3, Code 1)\n");
	                                break;

				case 2:
                                        printf("Destination protocol unreachable (Type 3, Code 2)\n");
                                        break;

                                case 3:
                                        printf("Destination port unreachable (Type 3, Code 3)\n");
                                        break;

                                case 4:
                                        printf("Fragmentation required, and DF flag set (Type 3, Code 4)\n");
                                        break;

                                case 5:
                                        printf("Source route failed (Type 3, Code 5)\n");
                                        break;

                                case 6:
                                        printf("Destination network unknown (Type 3, Code 6)\n");
                                        break;

                                case 7:
                                        printf("Destination host unknown (Type 3, Code 7)\n");
                                        break;

                                case 8:
                                        printf("Source host isolated (Type 3, Code 8)\n");
                                        break;

                                case 9:
                                        printf("Network administratively prohibited (Type 3, Code 9)\n");
                                        break;

                                case 10:
                                        printf("Host administratively prohibited (Type 3, Code 10)\n");
                                        break;

                                case 11:
                                        printf("Network unreachable for ToS (Type 3, Code 11)\n");
                                        break;

                                case 12:
                                        printf("Host unreachable for ToS (Type 3, Code 12)\n");
                                        break;

                                case 13:
                                        printf("Communication administratively prohibited (Type 3, Code 13)\n");
                                        break;

                                case 14:
                                        printf("Host Precedence Violation (Type 3, Code 14)\n");
                                        break;

                                case 15:
                                        printf("Precedence cutoff in effect (Type 3, Code 15)\n");
                                        break;

			}
			break;

		case 5:

			//Redireccion del mensaje
			printf("Redirect Message\n");

			//Existen varios casos
			switch(echoResponse.icmpHeader.Code){

				case 0:
					printf("Redirect Datagram for the Network (Type 5, Code 0)\n");
					break;

                                case 1:
                                        printf("Redirect Datagram for the Host (Type 5, Code 1)\n");
                                        break;

                                case 2:
                                        printf("Redirect Datagram for the ToS & network (Type 5, Code 2)\n");
                                        break;

                                case 3:
                                        printf("Redirect Datagram for the ToS & host (Type 5, Code 3)\n");
                                        break;

			}
			break;

		case 8:

			//Si nos enviamos un ping a nosotros mismos
			printf("Echo request (used to ping) (Type 8, Code 0)\n");
			break;

		case 9:

			//Problema con el router
			printf("Router Advertisement (Type 9, Code 0)\n");
			break;

		case 10:

			//Problema con el router
			printf("Router discovery/selection/solicitation (Type 10, Code 0)\n");
			break;

		case 11:

			//Tiempo excedido
			printf("Time Exceeded\n");

			//Varios casos
			switch(echoResponse.icmpHeader.Code){

				case 0:
					printf("TTL expired in transit (Type 11, Code 0)\n");
                       			break;

                                case 1:
                                        printf("Fragment reassembly time exceeded (Type 11, Code 1)\n");
                                        break;

			}
			break;

		case 12:

			//Problema cabecera IP
			printf("Parameter Problem: Bad IP header\n");

			//Varios casos
			switch(echoResponse.icmpHeader.Code){

				case 0:
					printf("Pointer indicates the error (Type 12, Code 0)\n");
					break;

                                case 1:
                                        printf("Missing a required option (Type 12, Code 1)\n");
                                        break;

                                case 2:
                                        printf("Bad length (Type 12, Code 2)\n");
                                        break;

			}
			break;

	}

	return 0;
}
