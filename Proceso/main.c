/* DMUTEX (2009) Sistemas Operativos Distribuidos
 * Código de Apoyo
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <unistd.h>

#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct mensajeReloj {
	  int tipoMensaje;
	  int posicionEnReloj;
	  int vectorReloj[20];
	  
} msgReloj;

int puerto_udp;
int posicionReloj;
int numeroProcesos;
int puertosUDP[20]= {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}; 
char *idsProcesos[20];
int reloj[20]= {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

int buscarClientePorPuerto(int puertoCliente){
	int k;
	for(k =0;k<numeroProcesos;k++){
		if(puertosUDP[k]==puertoCliente){
			return k;
		}
	}
	return (-1);
}

int buscarClientePorNombre(char *nombreCliente){
	int k;
	for(k =0;k<numeroProcesos;k++){
		if(!strcmp(idsProcesos[k],nombreCliente)){
			return k;
		}
	}
	return (-1);
}

int enviaMensajeAProceso(int tipo,char *identificadorProceso){

	fprintf(stdout,"Enviando mensaje al proceso %s \n", identificadorProceso);

	/* Busca puerto del proceso destino */
	int numProceso;
	numProceso = buscarClientePorNombre(identificadorProceso);
	if(numProceso == -1 ){
		perror("Proceso no conocido");
		return(-1);
	}
	
	fprintf(stdout,"Proceso %s encontrado, puerto %d \n", identificadorProceso, puertosUDP[numProceso]);
	
	msgReloj * mensajeAEnviar = malloc(sizeof(msgReloj));
	mensajeAEnviar->tipoMensaje = tipo;
	mensajeAEnviar->posicionEnReloj = posicionReloj;
	
	int k;
	for(k =0;k<numeroProcesos;k++){
		mensajeAEnviar->vectorReloj[k] = reloj[k];
	}	
	
	int sockfd;
	struct sockaddr_in servaddr;	
	
	/* Envia el mensaje */	
	sockfd=socket(AF_INET,SOCK_DGRAM,0);
	bzero(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr=inet_addr("127.0.0.1");
	servaddr.sin_port=htons(puertosUDP[numProceso]);
	
	fprintf(stdout,"Enviando el mensaje...\n");

	sendto(sockfd,&mensajeAEnviar,sizeof(msgReloj),0, (struct sockaddr *)&servaddr,sizeof(servaddr));

	fprintf(stdout,"Mensaje enviado\n");
	
	return 1;
}

int main(int argc, char* argv[]){

	int port;
	char line[80],proc[80];

	if(argc<2){
      fprintf(stderr,"Uso: proceso <ID>\n");
      return 1;
    }

	/* Establece el modo buffer de entrada/salida a línea */
	setvbuf(stdout,(char*)malloc(sizeof(char)*80),_IOLBF,80); 
	setvbuf(stdin,(char*)malloc(sizeof(char)*80),_IOLBF,80); 

	/* Crea socket udp de escucha */  	
	struct sockaddr_in dir;
	int socketIdent;
	if ((socketIdent=socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("error creando socket");
	}
	bzero(&dir,sizeof(dir));
	dir.sin_family=AF_INET;
	dir.sin_addr.s_addr=INADDR_ANY;	
	if (bind(socketIdent, (struct sockaddr *)&dir, sizeof(dir)) < 0) {
		perror("error en bind");
		close(socketIdent);	
	}
	socklen_t len = sizeof(dir);
	getsockname(socketIdent, (struct sockaddr *)&dir, &len);
	int puerto_udp;
	puerto_udp = ntohs(dir.sin_port);
	fprintf(stdout,"%s: %d\n",argv[1],puerto_udp);
	
	
	/* Rellena vectores de procesos y variables globales numeroProcesos, posicionReloj */
	int i=0;
	for(;fgets(line,80,stdin);){
		if(!strcmp(line,"START\n")){
			break;   
		}
		sscanf(line,"%[^:]: %d",proc,&port);
		
		if(!strcmp(proc,argv[1])){
			puertosUDP[i]=port;
			idsProcesos[i]=proc;
			posicionReloj = i;
		}
		else{
			puertosUDP[i]=port;
			idsProcesos[i]=proc;
		}
		i =i+1;
    }
	numeroProcesos = i;
	
	/* Inicializa Reloj */
	int k;
	for(k =0;k<numeroProcesos;k++){
		reloj[k]=0;
	}
	
	/* Procesa Acciones */
  	char mesg[50];
	for (;fgets(mesg,50,stdin);){
	
		char accion[20];
		char idProcesoDestino[50];
		sscanf(mesg,"%s %s",accion,idProcesoDestino);
		
		if(!strcmp(accion,"EVENT")){
		
			reloj[posicionReloj] = (reloj[posicionReloj])+1;
			fprintf(stdout,"%s: TICK\n",argv[1]);
		}
		else if(!strcmp(accion,"GETCLOCK")){
		
			fprintf(stdout,"%s: LC[",argv[1]);		
			for(k =0;k<numeroProcesos-1;k++){
				fprintf(stdout,"%d,",reloj[k]);
			}
			fprintf(stdout,"%d]\n",reloj[numeroProcesos-1]);
		}
		else if(!strcmp(accion,"RECEIVE")){
		
			/* Recibe el primer mensaje */
			
			msgReloj * mensajeRecibido = malloc(sizeof(msgReloj));
			struct sockaddr_in dirCliente;
			socklen_t fromlen = sizeof dirCliente;
			
			fprintf(stdout,"Recibiendo un mensaje...\n");			
			recvfrom(socketIdent,&mensajeRecibido,sizeof(msgReloj),0,(struct sockaddr *)&dirCliente,&fromlen);
			fprintf(stdout,"Mensaje recibido...\n");			
			
			int posicionCliente = mensajeRecibido->posicionEnReloj;
			
			fprintf(stdout,"Mensaje recibido del proceso en el puerto %d en posicion %d del array ... \n", puertosUDP[posicionCliente], posicionCliente);			
			
			if(mensajeRecibido->tipoMensaje == 1){
				fprintf(stdout,"%s: RECEIVE(MSG,%s)\n",argv[1], idsProcesos[posicionCliente]);
				
				/* Combina los relojes */
				for(k =0;k<numeroProcesos;k++){
					if(mensajeRecibido->vectorReloj[k]>reloj[k]){
						reloj[k] = mensajeRecibido->vectorReloj[k];
					}
				}							
			}
			else if(mensajeRecibido->tipoMensaje == 2){
				fprintf(stdout,"%s: RECEIVE(LOCK,%s)\n",argv[1], idsProcesos[posicionCliente]);
			}
			else if(mensajeRecibido->tipoMensaje == 3){
				fprintf(stdout,"%s: RECEIVE(OK,%s)\n",argv[1], idsProcesos[posicionCliente]);
			}
			
			/* Incrementa su reloj */
			reloj[posicionReloj] = (reloj[posicionReloj])+1;
			fprintf(stdout,"%s: TICK\n",argv[1]);
	
		}
		else if(!strcmp(accion,"LOCK")){
		
			/* Incrementa su reloj */
			reloj[posicionReloj] = reloj[posicionReloj]+1;
			fprintf(stdout,"%s: TICK\n",argv[1]);
			
			/* Envia mensaje al proceso de tipo 2 LOCK */	

			for(k =0;k<numeroProcesos;k++){
				if(k!=posicionReloj){
					enviaMensajeAProceso(2,idsProcesos[k]);
					fprintf(stdout,"%s: SEND(LOCK,%s)\n",argv[1], idsProcesos[k]);			
				}
			}
	
		}
		else if(!strcmp(accion,"UNLOCK")){
		}
		else if(!strcmp(accion,"MESSAGETO")){		
		
			/* Incrementa su reloj */
			reloj[posicionReloj] = reloj[posicionReloj]+1;
			fprintf(stdout,"%s: TICK\n",argv[1]);
			
			/* Envia mensaje al proceso de tipo 1 MSG*/			
			enviaMensajeAProceso(1,idProcesoDestino);
			fprintf(stdout,"%s: SEND(MSG,%s)\n",argv[1], idProcesoDestino);			
		}
		else{
			perror("Accion no permitida\n");
		}	
	}  
	return 0;
}
