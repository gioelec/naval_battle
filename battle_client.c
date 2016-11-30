#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>

#define INT_DIM 2
#define COD_WHO 1
#define COD_INFO 2
#define OK 3
#define COD_QUIT 5
#define USERNAME_LEN 30
#define COD_FAST_QUIT 7

int sd;
char username[USERNAME_LEN];

enum StatoCasella{OCCUPATA,COLPITA,MANCATA,VUOTO};
// nave posizionata o //nave non colpita ~ // nave colpita     x	// Vuoto ?
void controllaReceive(int ret){
	if(ret==-1){
		perror("Errore nella recv");
		exit(1);
	}	
}
void stampa(enum StatoCasella *b){
	int i=0;
	int riga=0;
	printf("  A B C D E F");
	for(;i<36;i++){
		if((i%6)==0)
			printf("\n%d ",++riga);
		switch(b[i]){
			case OCCUPATA:
				printf("o ");
				break;
			case MANCATA: 
				printf("~ ");
				break;			
			case COLPITA:
				printf("x ");
				break;
			case VUOTO:
				printf("? ");
				break;
		}
	}
	printf("\n");
	printf("\n");
}
int ottieniX(char *buf){
	if(strcmp("A",buf)==0)
		return 1;
	if(strcmp("B",buf)==0)
		return 2;
	if(strcmp("C",buf)==0)
		return 3;
	if(strcmp("D",buf)==0)
		return 4;
	if(strcmp("E",buf)==0)
		return 5;
	if(strcmp("F",buf)==0)
		return 6;
	return -1;
}
void resettaGriglia(enum StatoCasella*b){
	int i;
	for ( i = 0; i < 36; ++i)
	{
		b[i]=VUOTO;
	}
}
void posizionaNavi(enum StatoCasella *b){
	resettaGriglia(b);
	int i=0;
	int x,y;
	char buf[4];
	printf("Inserire le coordinate in cui posizionare le 7 navi indicando le caselle [A-F],[1-6] separate da virgola\n");
	
	while (i<7){
		printf(">");
		scanf("%s",buf);
		if(strlen(buf)!=3){
			printf("casella non valida \n");
			continue;
		}
		y=atoi((const char*)&buf[2]);
		char *p=strtok(buf,",");
		if(p==NULL){
			printf("casella non valida \n");
			continue;
		}
		x=ottieniX(p);
		if(x==-1 || y>6 || y<1){
			printf("casella non valida \n");
			continue;
		}
		if(	b[x-1+(y-1)*6]!=VUOTO){
			printf("nave già posizionata in questa casella \n");
			continue;
		}
		b[x-1+(y-1)*6]=OCCUPATA;
		i++;
	}
}
void inviaInt(int sd, int msgl){
	int ret;
	printf("messaggio prima htonl%d\n",msgl );
	int msg=htonl(msgl);
	printf("messaggio dopo htonl%d\n",ntohl(msg ));
	ret=send(sd,(void*)&msg,sizeof(int),0);
	printf("%d\n",ret );
	if(ret==-1){
		perror("Errore nell'invio dell'intero al client");
		exit(1);
	}
}
void help(){
	printf("\nSono disponibili i seguenti comandi:\n!help --> mostra l'elenco dei comandi disponibili \n!who --> mostra l'elenco dei client connessi al server\n!connect username --> avvia una partita con l'utente username\n!quit --> disconnette il client dal server\n");	
}
int quantiByte(int i){
	int ret;
	uint32_t dimMsg;
	ret=recv(i,(void*)&dimMsg,sizeof(int),0);
	controllaReceive(ret);
	int dimMsg2=(int)ntohl(dimMsg);
	printf("byte ricevuti: %d \n",ret);
	printf("il server vuole mandare %d byte\n",dimMsg2 );
	return dimMsg2;
}
void riceviByte(int i, void*buf,int dimMsg){
	int ret;
	ret=recv(i,(void*)buf,dimMsg,0);
	controllaReceive(ret);
	printf("byte ricevuti: %d \n \n",ret);
}
void inviaByte(int sd, int dim, void * msg){
	int ret;
	inviaInt(sd,dim);
	ret=send(sd,msg,dim,0);
	if(ret==-1){
		perror("Errore nell'invio delle informazioni al server");
		exit(1);
	}
}
int insertUsername(char*user){
    scanf("%s",username);
    int dimMsg=strlen(username);
    if(dimMsg>=USERNAME_LEN){
        dimMsg=USERNAME_LEN;
        username[USERNAME_LEN-1]='\0';
        printf("username troppo lungo verrà inviato %s\n",user);
    }
    else
        dimMsg++;
    return dimMsg;
}
void who(int sd){
	printf("provo invio al server il codice %d \n",COD_WHO);
	inviaInt(sd,COD_WHO);
	int dimMsg=quantiByte(sd);
	char buf[dimMsg];
	riceviByte(sd,buf,dimMsg);
	printf("Clienti connessi al server:\n");
	printf("%s\n",buf );
}
void quit(int sd,char*username){
	printf("provo invio al server il codice %d \n",COD_QUIT);
	inviaInt(sd,COD_QUIT);
	int dimMsg=strlen(username)+1;
	inviaByte(sd,dimMsg,username);
	printf("Mi disconnetto dal server:\n");
	close(sd);
}
void inserisciComando(int sd,char *buf,char*username){
		printf("> ");
		fflush(stdout);
		scan:
		scanf("%s",buf);
		if(strcmp("!quit",buf)==0){
			quit(sd,username);
			exit(0);
		}
		if(strcmp("!help",buf)==0){
			help();
			return;//continue;
		}
		if(strcmp("!connect",buf)==0){
			scanf("%s",buf);
			if(strcmp("username",buf)==0)
				printf("da fare probabilmente bisogna fare una doppia send e receive con il server");
				return;//continue;
		}
		if(strcmp("!who",buf)==0){
			printf("compare riconosciuto who\n");
			who(sd);
			return;//continue;
		}
		printf("Comando digitato non riconosciuto, riprovare\n");
		goto scan;
}
void inserisciPorta(int * portaAscolto){
	while(1){
		printf("Inserisci la tua porta UDP di ascolto: ");
		scanf("%d",portaAscolto);
		if(*portaAscolto<1025||*portaAscolto>0xFFFF){
			printf("Porta indicata non valida, RIPROVARE\n");
			continue;
		}
		break;
	}
}
void inviaInfo(int sd,char *username){
	int portaAscolto,ret;
	while(1){
		printf("Inserisci il tuo nome client: ");
		int dimMsg=insertUsername(username);
		inserisciPorta(&portaAscolto);
		inviaInt(sd,COD_INFO);
		inviaInt(sd,portaAscolto);
		printf("invio :%s\n",username );
		inviaByte(sd,dimMsg,username);
		int p=quantiByte(sd);
		if(p==OK){
			printf("Username accettato dal server\n");
			break;
		}
		printf("codice di ritorno %d\n",p );
		printf("username già presente sul server: %s\n", username );
	}
}
void mysigint(){
	printf("entro nella mysigint\n");
	if(strcmp(username,"")==0){
		printf("esco senza cancellarmi\n");
		inviaInt(sd,COD_FAST_QUIT);
		close(sd);
		exit(0);
	}
	quit(sd,username);
	exit(0);
}

int main(int argc,char* argv[]) {
    strcpy(username,"");
    if (signal(SIGINT, mysigint) == SIG_ERR)
        printf("Cannot handle SIGINT!\n");
    if(argc<2){
        printf("Errore passaggio argomenti al server\n");
        exit(1);
    }
    int porta=atoi((const char*)argv[2]);
    char * ip=argv[1];
    int ret/*,sd*/;
    sd=socket(AF_INET,SOCK_STREAM,0);
    if(sd==-1){
        perror("Errore nella creazione del socket ");
        exit(1);
    }
    struct sockaddr_in server;
    memset(&server,0,sizeof(server));
    server.sin_family=AF_INET;
    server.sin_port=htons(porta);
    inet_pton(AF_INET,ip,&server.sin_addr);

    ret=connect(sd,(struct sockaddr*)&server,sizeof(server));
    if(ret==-1){
        printf("connessione al server %s (porta %d) fallita \n",ip,porta);
        perror("Errore nella connessione ");
        exit(1);
    }
    printf("connessione al server %s (porta %d) riuscita\n",ip,porta);
    //char username[USERNAME_LEN];
    help();
    inviaInfo(sd,username);
    
    char buf[20];
    fd_set master;
    fd_set read;
    int fdmax;
    FD_ZERO(&master);
    FD_ZERO(&read);

    FD_SET(sd,&master);
    FD_SET(0,&master);
    fdmax=sd;
    int i;
    for(;;){
    	printf("> ");
        fflush(stdout);
        read=master;
        select(fdmax+1,&read,NULL,NULL,NULL);
        for(i=0;i<=fdmax;i++){
            if(FD_ISSET(i,&read)){
                if(i==sd){
                    ///richiesta dal server
                }else if (i==0){
                    inserisciComando(sd,buf,username);
                }
            }
    
        }

    }






/*
// dati per la battaglia
    int naviRimaste=7;
    enum StatoCasella tabella[36];
    posizionaNavi(tabella);
    stampa(tabella);*/
    return 0;
}
