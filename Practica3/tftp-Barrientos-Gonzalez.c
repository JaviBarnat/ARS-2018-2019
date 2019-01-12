// Practica tema 7, Barrientos Gonzalez Javier

#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char* argv[]) {

	char datos[516]; //Buffer de datos
	char ack[4]; //Buffer de ACK
	int bandera = 0; //El estado de -v como opcion
	struct in_addr addr;
	int d; //Numero de bloque

	int lectura = 0; //Bandera sobre el estado del fichero, 0 escribimos, 1 leemos

	struct servent *serverPort;
	int puerto; //Almacenamos el numero de puerto

	switch(argc){

		case 4:
			//Comprobamos que la direccion pasada es correcta y que la opcion sea valida
			if(inet_aton(argv[1], &addr) == 0){
				printf("Direccion IP no valida. Pruebe con una direccion valida.\n");
				exit(EXIT_FAILURE);
			}

			//Comprobamos que las opciones son correctas
			if(strcmp(argv[2], "-r") != 0 && strcmp(argv[2], "-w") != 0){
				printf("Opcion no valida. Inserte una opcion valida {-r|-w}.\n");
				exit(EXIT_FAILURE);
			}

			//Si se va a leer el fichero activamos la bandera correspondiente para su futuro uso
			if(strcmp(argv[2], "-r") == 0){
				lectura = 1;
			}
			break;

		case 5:
			//Comprobamos que la direccion pasada es correcta y que la opcion tambien lo sea
                        if(inet_aton(argv[1], &addr) == 0){
                                printf("Direccion IP no valida. Pruebe con una direccion valida.\n");
                                exit(EXIT_FAILURE);
                        }

			//Comprobamos que las opciones insertadas son correctas
                        if(strcmp(argv[2], "-r") != 0 && strcmp(argv[2], "-w") != 0){
                                printf("Opcion no valida. Inserte una opcion valida {-r|-w}.\n");
                                exit(EXIT_FAILURE);
                        }

			//Se comprueba que la opcion correcta es -v en la quinta posicion del programa
			if(strcmp(argv[4], "-v") != 0){
				printf("Opcion no valida. Inserte una opcion valida {-v}.\n");
				exit(EXIT_FAILURE);
			}

			//Si se va a leer el fichero activamos la bandera correspondiente para su futuro uso
                        if(strcmp(argv[2], "-r") == 0){
                                lectura = 1;
                        }

			//Activamos el modo -v
			bandera = 1;
                        break;

		default:

			//Si no se cumple el resto de condiciones error y finalizacion del programa
			printf("Composicion del comando incorrecta: ./tftp-Barrientos-Gonzalez direccion-ip {-r|-w} nombre-fichero [-v].\n");
			exit(EXIT_FAILURE);
			break;
	}

	//Definimos el socket
	int sock;
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock<0){
		//Imprimos el codigo de error y paramos la ejecucion
		perror("socket()");
		exit(EXIT_FAILURE);
	}

	//Obtenemos el puerto por defecto referido a TFTP en UDP
	serverPort = getservbyname("tftp","udp");
	puerto = serverPort->s_port;

	//Estructura referida al cliente para usar el bind()
	struct sockaddr_in client;
	client.sin_family = AF_INET;
        client.sin_port = 0;
        client.sin_addr.s_addr = INADDR_ANY;

	//Estructura referida al servidor
	struct sockaddr_in server;
        server.sin_family = AF_INET;
        server.sin_port = puerto;
        server.sin_addr = addr;
	socklen_t serverL = sizeof(server);

	//Creamos el bind para conectar el socket con la estructura del cliente creada con anterioridad
	int err;
	err = bind(sock, (struct sockaddr*) &client, sizeof(client));
	if(err<0){
		//Mostramos el error por pantalla y finalizamos la ejecucion
		perror("bind()");
		exit(EXIT_FAILURE);
	}

	//Comprobamos la bandera de lectura
	//Si presenta un 1 significa que se va a leer el fichero
	if(lectura == 1){

		//Cremos y rellenamos el buffer de peticion de envio para asi poder usar
		//strcat para concatenar el contenido ya que este pide contener
		//cadenas para poder usarlo
		char rrq[200]="aa";
		strcat(rrq, argv[3]);

		//Rellenamos el contenido para añadir luego un cero en la posicion corercta
		rrq[2+strlen(argv[3])]=123;
		strcat(rrq, "octet"); //Modo
		rrq[2+strlen(argv[3])]=0;

		//Sustituimos los valores iniciales por los necesarios (OP RRQ = 01)
		rrq[0]=0;
		rrq[1]=1;

		//Enviamos los datos al servidor
		int snd;
		snd = sendto(sock, rrq, 200, 0, (struct sockaddr*) &server, sizeof(server));
		if(snd<0){
			//Mostramos el error y finalizamos la ejecucin del programa
			perror("sendto()");
			exit(EXIT_FAILURE);
		}

		//Se imprime los pasos realizados
		if(bandera == 1){
			printf("Enviada solicitud de lectura de %s a servidor tftp en %s \n", argv[3], argv[1]);
		}

		//Creamos las variables para su utilizacion en el bucle
		int llegada;
		int llegada_estimada=0;
		FILE *fichero = fopen(argv[3], "w");

		//Bucle para recibir los datos del servidor
		do{

			//Recibimos el paquete de datos que almacenamos en el buffer datos
			llegada=recvfrom(sock, datos, 516, 0, (struct sockaddr *) &server, &serverL);
			if(llegada<0){
				//Si hay algun error lo mostramos y detenemos la ejecucion
				perror("recvfrom()");
				exit(EXIT_FAILURE);
			}

			//Obtenemos el valor del bloque actual con los valores de los campos presentes en la cabecera
			//recibida desde el servidor
			d=((unsigned char)datos[2]*256+(unsigned char)datos[3]);
			llegada_estimada++; //Sumamos 1 al valor del contador para comprobar que los paquetes llegan en orden

			//Comprobamos que los paquetes llegan de manera correcta y ordenada seggun su NumBlock
			if(llegada_estimada != d){
				printf("Paquetes en orden no correcto.\n");
				exit(EXIT_FAILURE);
			}

			//Si el paquete recibido tiene un valor 05 en la cabecera se trata de un mensaje de error que debemos
			//de mostrar por pantalla con la informacion correspondiente
			if(datos[1] == 5){
				printf("Error %i%i\n", datos[2], datos[3]);
				exit(EXIT_FAILURE);
			}

			//Si la opcion -v esta activada, realizamos la impresion de los pasos que se estan realizando dentro
			//de este bucle
			if(bandera == 1){
				if(d == 1){
					printf("Es el primer bloque (numero de bloque 1).\n");
				}else{
					printf("Recibido bloque del servidor tftp.\nEs el bloque con codigo %i.\n", d);
				}
			}

			//Escribimos los datos recibidos del paquete en el fichero con el mismo nombre que le hemos dado
			fwrite(datos+4, 1, llegada-4, fichero);

			//Preparamos el paquete ACK con el valor del bloque recibido para solicitar la siguiente parte de este
			ack[0]=0;
			ack[1]=4, //Codigo 04
			ack[2]=datos[2]; //Numero de bloque
			ack[3]=datos[3];

			//Creamos la estructura para enviar el ACK al servidor
			int s;
			s = sendto(sock, ack, 4, 0, (struct sockaddr *) &server, sizeof(server));
			if(s<0){
				//Si existe un error mostramos este por pantalla y detenemos la ejecucion
				perror("sendto()");
				exit(EXIT_FAILURE);
			}

			//Si tenemos la opcion -v activada mostramos los pasos que recorre el ACK
			if(bandera == 1){
				printf("Enviamos el ACK del bloque %i.\n", d);
			}

		}while(llegada==516);

		//Si -v estÃ¡ activado mostramos el ultimo bloque del mensaje como final
		if(bandera == 1){
			printf("El bloque %i era el ultimo. Cerramos el fichero.\n", d);
		}

		//Cerramos el fichero abierto para la ocasion
		fclose(fichero);


	}

	//Comprobamos la bandera de lectura
	//Si presenta un 0 significa que se va a escribir el fichero
	if(lectura == 0){

		//Cremos y rellenamos el buffer de peticion de envio para asi poder usar
                //strcat para concatenar el contenido ya que este pide contener
                //cadenas para poder usarlo
                char wrq[200]="aa";
                strcat(wrq, argv[3]);

                //Rellenamos el contenido para aï¿½adir luego un cero en la posicion corercta
                wrq[2+strlen(argv[3])]=123;
                strcat(wrq, "octet"); //Modo
                wrq[2+strlen(argv[3])]=0;

                //Sustituimos los valores iniciales por los necesarios (OP WRQ = 02)
                wrq[0]=0;
                wrq[1]=2;

                //Enviamos los datos al servidor
                int snd;
                snd = sendto(sock, wrq, 200, 0, (struct sockaddr*) &server, sizeof(server));
                if(snd<0){
                        //Mostramos el error y finalizamos la ejecucin del programa
                        perror("sendto()");
                        exit(EXIT_FAILURE);
                }

                //Se imprime los pasos realizados
                if(bandera == 1){
                        printf("Enviada solicitud de escritura de %s a servidor tftp en %s \n", argv[3], argv[1]);
                }

		//Definimos el fichero del sistema que queremos leer para transmitir al servidor
		FILE *fichero = fopen(argv[3], "r");
		int contador = 0;
		char caracter;
		int numero_estimado=0;

		//Bucle encargado de leer todo el fichero para enviar su contenido al servidor
		while(!feof(fichero)){

			//Leermos el fichero y vamos guardando su contenido caracter a caracter
			caracter=fgetc(fichero);
			datos[contador%512+4] = caracter;
			contador++;

			//Si el buffer ha llegado a 512 o si es final de fichero debemos enviar lo obtenido
			if(contador%512 == 0 || feof(fichero)){

				//Recibimos el ACK del servidor
				int recc;
				recc = recvfrom(sock, ack, 4, 0, (struct sockaddr*) &server, &serverL);
				if(recc<0){
					//Mostramos el error por pantalla y detenemos la ejecucion del programa
					perror("recvfrom()");
					exit(EXIT_FAILURE);
				}

				//Comprobamos que no es un mensaje de error y si lo es mostramos su contenido por pantalla
				if(ack[1] == 5){
					printf("Error %i%i\n", ack[2], ack[3]);
	                                exit(EXIT_FAILURE);
				}

				//Obtenemos el valor del bloque a partir de los datos recibidos del ACK
				d=((unsigned char)ack[2]*256+(unsigned char)ack[3]+1);
				numero_estimado++;

				if(numero_estimado != d){
                                	printf("Paquetes en orden no correcto.\n");
                                	exit(EXIT_FAILURE);
                        	}

				//Si la opcion -v esta activa
				if(bandera == 1){
					printf("Recibimos el ACK del bloque %i.\n", d-1);
				}

				//Calculamos el tamanyo del mensaje a enviar
				int tamanyo;
				if(contador%512 == 0){
					//Si el resto de dividir el contador entre 512 nos da cero significa que el bloque leido no es referido al
					//ultimo bloque a enviar por lo tanto su tamanyo final sera 516 (con cabecera)
					tamanyo=516;
				}else{
					//En cambio si es el ultimo fichero el resto es diferente a 0 y el mensaje debe ser
					//estrictamente menor a 512 (sin contar la cabecera) por ello el tamanyo maximo
					//en este caso sera
					tamanyo=contador%512+3;
				}

				//Estructuramos el paquete de datos que vamos a mandar
				datos[0]=0;
				datos[1]=3;
				datos[2]=d/256;
				datos[3]=d%256;

				//Enviamos el paquete al servidor
				int sendData;
				sendData = sendto(sock, datos, tamanyo, 0, (struct sockaddr *) &server, sizeof(server));
				if(sendData<0){
					//Mostramos el error por pantalla y finalizamos el programa
					perror("sendto()");
					exit(EXIT_FAILURE);
				}

				//Si tenemos la opcion -v activa
				if(bandera == 1){
					if(contador == 1){
						printf("Enviamos el primer bloque.\n");
					}else{
						printf("Se ha enviado el bloque %i.\n",d);
					}
				}
			}
		}

		//Si tenemos activo la opcion -v
		if(bandera == 1){
			printf("Enviamos el ultimo bloque %i.\n", d);
		}

		//Cerramos el fichero a enviar
		fclose(fichero);
	}

	//Cerramos el socket
	close(sock);

	//Finalizamos el programa
	return 0;
}
