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

int main(int argc, char *argv[]){

	//Definimos los valores iniciales del puerto
        int puerto = 0;

        //Comprobamos los argumentos pasados por comando
        struct in_addr addr; //Estructuta para guardar la di
        struct servent *serverPort; //Estrcutura para guarda

	//Seleccion de accion dependiendo del Ânumero de arguemntos
	switch(argc){
                //Presentamos unicamente la direccion IP del servidor
                case 2:
                        //Comprobamos que la direccion IP pasada como argumento es valida
                        //Si lo es, la pasamos ya convertida a addr, si no, salta el error
                        if(inet_aton(argv[1], &addr) == 0){
                                printf("Direccion IP no valida. Pruebe con una IP valida.\n");
                                exit(EXIT_FAILURE);
                        }
                        //A partir del servicio QOTD, obtenemos el puerto por defecto y lo guardamos
                        serverPort = getservbyname("qotd", "tcp");
                        puerto = serverPort->s_port;
                        break;

                //Presentamos la direccion IP, la opcion de puerto (-p) y el numero de puerto del servidor
                case 4:
                        //Comprobamos que la direccion IP pasada como argumento es valida
                        //Si lo es, la pasamos ya convertida a addr, si no, salta el error
                        if(inet_aton(argv[1], &addr) == 0){
                                printf("Direccion IP no valida. Pruebe con una IP valida.\n");
                                exit(EXIT_FAILURE);
                        }
                        //Comprobamos que la opcion insertada es la referente al puerto (-p)
                        //Si no lo es, salta el error
                        if(strcmp(argv[2], "-p") != 0){
                                printf("Opcion no valida. Inserte una opcion valida.\n");
                                exit(EXIT_FAILURE);
                        }
                        //Obtenemos el numero de puerto pasado como argumento y lo pasamos a
                        //network byte order con htons
                        sscanf(argv[3], "%d", &puerto);
                        puerto = htons(puerto);
                        break;

                //Si no se cumple ninguna de las anteriores, devuelve un mensaje de error comun con
                //la estructura base del comando y paramos la ejecicion

		default:
                        printf("Composicion del comando incorrecta: ./qotd-udp-client direccionIP [-p puerto-server].\n");
                        exit(EXIT_FAILURE);
                        break;
        }

	//Creamos el socket al cual vamos a enlazar la peticion al servidor UDP
        int sock;
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock<0) {
                //Imprimimos el codigo de error y paramos si el socket no ha sido creado correctamente
                perror("socket()");
                exit(EXIT_FAILURE);
        }

	//Estructura referida al cliente para usar en bind() y recivfrom()
        //Asignamos el puerto, la familia y la direccion IP del cliente
        struct sockaddr_in client;
        client.sin_family = AF_INET;
        client.sin_port = 0;
        client.sin_addr.s_addr = INADDR_ANY;

        //Estructura referida al servidor para usar sendto()
        //A partir de los valores obtenidos anteriormente, aÃ±adimos el puerto destino
        //y la direccion IP final
        struct sockaddr_in server;
        server.sin_family = AF_INET;
        server.sin_port = puerto;
        server.sin_addr = addr;

        //Creamos el bind() para conectar nuestro socket a la estructura del cliente
        //creada con anterioridad
        int err;
        err = bind(sock, (struct sockaddr *) &client, sizeof(client));
        if (err < 0){
                //Imprimimos el codigo de error y paramos si el bind() no ha sido creado correctamente
                perror("bind()");
                exit(EXIT_FAILURE);
        }

	//Conectamos el cliente con el servidor usando la informacion proporcionada en el comando
	int con;
	con = connect(sock, (struct sockaddr *) &server, sizeof(server));
	if (con < 0){
		//Imprimimos el codigo de error y paramos si ha habido un fallo al conectar con el servidor
		perror("connect()");
		exit(EXIT_FAILURE);
	}

	//Creamos el buffer donde almacenamos el mensaje recibido del servidor
	char buffer[500];

	//Recibimos el mensaje desde el socket y lo guardamos en el buffer
	int rec;
	rec = recv(sock, buffer, sizeof(buffer), 0);
	if (rec < 0){
		//Imprimimos el codigo de error y paramos si ha habido un fallo al recibir el mensaje
		perror("recv()");
		exit(EXIT_FAILURE);
	}

	//Imprimimos el mensaje recibido por pantalla
	printf("%s", buffer);

	//Cerramos el socket
	int sut;
	sut = shutdown(sock, SHUT_RDWR);
	if(sut < 0){
		//Imprimimos el codigo de error y paramos la ejecucion si hay un fallo
		perror("shutdown()");
		exit(EXIT_FAILURE);
	}
	close(sock);

	//Finalizamos el programa
	return 0;
}
