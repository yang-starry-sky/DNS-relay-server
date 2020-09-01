#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "head.h"

#pragma comment(lib, "Ws2_32.lib")

void listTitle() {
	printf("Class:2018211319\n");
	printf("TeamMember:Qiao Sen,Wang Mingzi,Yang Chengdong\n");
	printf("\n");

	printf("DNSRELAY, Version 1.00\n");
	printf("Usage: dnsrelay [-d | -dd] [<dns-server>] [<db-file>]\n");
	printf("\n");
}

void readParameters(int argc, char* argv[]) {
	bool setDNSFlag = false;
	bool setFilePath = false;
	if (argc > 1 && argv[1][0] == '-') {
		if (argv[1][1] == 'd')
			debugLevel = 1;
		if (argv[1][2] == 'd')
			debugLevel = 2;
	}

	if (argc > 2) {
		setDNSFlag = true;
		strcpy(outDNSServerIP, argv[2]);
	}

	if (argc > 3) {
		setFilePath = true;
		strcpy(filePath, argv[3]);
	}

	if (setDNSFlag)
		printf("Set Out DNS Server IP:%s\n", outDNSServerIP);
	else
		printf("Use Default Out DNS Server IP:%s\n", outDNSServerIP); 

	if (setFilePath)
		printf("Set File Path:%s\n", filePath);
	else
		printf("Use Default File Path:%s\n", filePath);

	printf("Debug level : %d\n", debugLevel);
}
void readIPURLReflectTable() {
	FILE* fp = NULL;
	if ((fp = fopen(filePath, "r+")) == NULL) {
		perror("fail to read!!!");
		exit(1);
	}
	while (!feof(fp))
	{
		fscanf(fp, "%s %s", IPTable[CurrentTableNumber], URLTable[CurrentTableNumber]);
		if (debugLevel >= 2)
			printf("IP:%s->URL:%s\n", IPTable[CurrentTableNumber], URLTable[CurrentTableNumber]);
		CurrentTableNumber++;
		TotalTableNumber++;
	}
	if (debugLevel >= 0)
		printf("��ȡ�ɹ�\n");
		if (debugLevel >= 1)
		printf("��ȡ������Ϊ��%d\n", TotalTableNumber);

	fclose(fp);
}
void outPutCurrentTime()
{
	time_t t;
	struct tm* lt;   
	time(&t);//��ȡʱ�����    
	lt = localtime(&t);//תΪʱ��ṹ��    
	printf("%d/%d/%d %d:%d:%d", lt->tm_year + 1900, lt->tm_mon+1, lt->tm_mday,lt->tm_hour, lt->tm_min, lt->tm_sec);//������
}
void initTransferTable() {
	int i = 0;
	for (; i < MAX_ID_TARNSFER_TABLE_LENGTH; i++)
		isDone[i] = true;
}
//����Ϊ6github3com0��urlתΪgithub.com��ʽ��requestIP
void convertToURL(char* url, char* requestURL)
{
	int i = 0, j = 0, k = 0, len = strlen(url);
	while (i < len)
	{
		if (url[i] > 0 && url[i] <= 63) //���url������(0,64)֮��
		{
			for (j = url[i], i++; j > 0; j--, i++, k++) //���Ƹ�url
				requestURL[k] = url[i];
		}
		if (url[i] != 0) //����Ƿ����������β�Ƿ�Ϊ0
		{
			requestURL[k] = '.';
			k++;
		}
	}
	requestURL[k] = '\0'; 
}
void receiveFromOut()
{
	char buffer[MAX_BUF_SIZE], requestURL[MAX_URL_LENGTH];
	//��BUF����
	for (int i = 0; i < MAX_BUF_SIZE; i++)
		buffer[i] = 0;
	int bufferLength = -1;
	bufferLength = recvfrom(outSocket, buffer, sizeof(buffer), 0, (struct sockaddr*) & out, &lengthClient); //���ⲿ����DNS���ݰ�
	if (bufferLength > -1)//�������DNS���ݰ�
	{
		unsigned short* newID = (unsigned short*)malloc(sizeof(unsigned short));
		memcpy(newID, buffer, sizeof(unsigned short));//��ȡ���ݰ�ͷ����ID����ת�����ID
		int index = (*newID) - 1;//��ת�����IDת��ΪIDת�����������������
		free(newID);//�ͷſռ�
		memcpy(buffer, &oldIDtable[index], sizeof(unsigned short));//ʹ�þ�ID�滻ת�����ID
		isDone[index] = TRUE;//�������󣬽�isDone���ж�Ӧ��������Ϊtrue
		client = IDClient[index];//��ȡ�ͻ��˷�����
		bufferLength = sendto(localSocket, buffer, bufferLength, 0, (SOCKADDR*)& client, sizeof(client));
		int QDCount = ntohs(*((unsigned short*)(buffer + 4))), ANCount = ntohs(*((unsigned short*)(buffer + 6)));
		char* bufferLocation = buffer + 12;//����DNS��ͷ��ָ��
		char ip[16];
		int ipPart1, ipPart2, ipPart3, ipPart4;
		for (int i = 0; i < QDCount; i++)
		{
			convertToURL(bufferLocation, requestURL);
			while (*bufferLocation > 0)//��ȡ��ʶ��ǰ�ļ����������url
				bufferLocation += (*bufferLocation) + 1;
			bufferLocation += 5; //����url�����Ϣ��ָ����һ������
			for (int i = 0; i < ANCount; ++i)
			{
				if ((unsigned char)* bufferLocation == 0xc0) //����Ƿ�Ϊָ��
					bufferLocation += 2;
				else 
				{
					while (*bufferLocation > 0)
						bufferLocation += (*bufferLocation) + 1;
					++bufferLocation;//ָ��url���������
				}
				unsigned short responseType = ntohs(*(unsigned short*)bufferLocation);  //�ظ�����
				bufferLocation += 2;
				unsigned short responseClass = ntohs(*(unsigned short*)bufferLocation); //�ظ���
				bufferLocation += 2;
				unsigned short responseHighTTL = ntohs(*(unsigned short*)bufferLocation);//����ʱ���λ
				bufferLocation += 2;
				unsigned short responseLowTTL = ntohs(*(unsigned short*)bufferLocation); //����ʱ���λ
				bufferLocation += 2;
				int TTL = (((int)responseHighTTL) << 16) | responseLowTTL;    //��ϳ�����ʱ��
				int dataLength = ntohs(*(unsigned short*)bufferLocation);   //���ݳ���
				bufferLocation += 2;
			
				if (responseType == 1) //�����A����
				{
					ipPart1 = (unsigned char)* bufferLocation++;
					ipPart2 = (unsigned char)* bufferLocation++;
					ipPart3 = (unsigned char)* bufferLocation++;
					ipPart4 = (unsigned char)* bufferLocation++;

					sprintf(ip, "%d.%d.%d.%d", ipPart1, ipPart2, ipPart3, ipPart4);//�� ipPart1, ipPart2, ipPart3, ipPart4ƴ��ΪIP��ַ
				}

				//��URL��IP��ŵ�Cache��
				strcpy(URLCache[CacheCount], requestURL);
				strcpy(IPCache[CacheCount], ip);
				CacheCount++;
				//���cache���Ѵ��������cache�����λ��ʼ�滻
				if (CacheCount == MAX_CACHE_LENGTH)
					CacheCount = 0;
				TotalCacheNumber++;
				if (TotalCacheNumber > MAX_CACHE_LENGTH)
					TotalCacheNumber = MAX_CACHE_LENGTH;
				if (debugLevel >= 1)
				{
					
						cout << Number << ":  ";
						outPutCurrentTime();
						cout << "  " << "Client" << "  " << "127.0.0.1" << "     " << "1.0.0.127.in-addr.arpa, TYPE 12 ,CLASS 1" << endl;
						Number++;
						cout << Number << ":  ";
						outPutCurrentTime();
						cout << "  " << "Client" << "  " << "127.0.0.1" << "     " << requestURL << endl;
						IPV4 = false;
						Number++;
						char outBuffer[2];
						if (debugLevel >= 2 ) {
							cout << "----------------------------------------" << endl;
							cout << "Type :  " << responseType << "  Class :  " << responseClass << "  TTL :  " << TTL << "  dataLength :  " << dataLength << endl;
							cout << "buffer :  ";
							for (int i = 0; i < bufferLength; i++) {
								_itoa((unsigned short)buffer[i], outBuffer, 16);
								cout << outBuffer[0] << outBuffer[1] << " ";
							}
							cout << endl;
							cout << "----------------------------------------" << endl;
						}
					}
				
				
			}
		}
	}

}
void receiveFromLocal()
{
	char buffer[MAX_BUF_SIZE], url[MAX_URL_LENGTH];
	//��BUF����
	for (int i = 0; i < MAX_BUF_SIZE; i++)
		buffer[i] = 0;
	int bufferLength = -1;
	char currentIP[MAX_IP_LENGTH];
	bufferLength = recvfrom(localSocket, buffer, sizeof buffer, 0, (struct sockaddr*) & client, &lengthClient);
	//����ѯ������
	char requestURL[MAX_URL_LENGTH];
	
	if (bufferLength > 0) {
		//��buffer�е��������ִ���url��
		memcpy(url, &(buffer[12]), bufferLength);
		convertToURL(url, requestURL);

		//��Cache�в�ѯ������
		for (CurrentCacheNumber = 0; CurrentCacheNumber < TotalCacheNumber; CurrentCacheNumber++)
		{
			if (strcmp(requestURL, URLCache[CurrentCacheNumber]) == 0)
				break;
		}
		//���δ��Cache���ҵ�������
		if (CurrentCacheNumber == TotalCacheNumber)
		{
			//��IP-URL���ձ��в��Ҹ�����
			for (CurrentTableNumber = 0; CurrentTableNumber < TotalTableNumber; CurrentTableNumber++) {
				if (strcmp(requestURL, URLTable[CurrentTableNumber]) == 0)
					break;
			}
			//δ����IP-URL���ձ��в鵽
			if (CurrentTableNumber == TotalTableNumber)
			{
				//���ⲿDNS�����������ѯ������
				for (CurrentIDNumber = 0; CurrentIDNumber < MAX_ID_TARNSFER_TABLE_LENGTH; CurrentIDNumber++)
					if (isDone[CurrentIDNumber] == true)
						break;
				if (CurrentIDNumber == MAX_ID_TARNSFER_TABLE_LENGTH)
				{
					;//����
				}
				else {
					unsigned short* oldID = (unsigned short*)malloc(sizeof(unsigned short));
					memcpy(oldID, buffer, sizeof(unsigned short)); 
					//��oldID����IDת����
					oldIDtable[CurrentIDNumber] = *oldID;
					//��isDone��Ϊfalse
					isDone[CurrentIDNumber] = false;
					IDClient[CurrentIDNumber] = client;
					CurrentIDNumber += 1;
					memcpy(buffer, &CurrentIDNumber, sizeof(unsigned short));
					bufferLength = sendto(outSocket, buffer, bufferLength, 0, (struct sockaddr*) & externName, sizeof(externName));//���ⲿ���������Ͳ�ѯ����
				}
			}
			else {

				strcpy(currentIP, IPTable[CurrentTableNumber]);
				//��ѯ�������Ƿ��ں�������
				char sendbuf[MAX_BUF_SIZE];
				int currenLength = 0;
				if ((strcmp("0.0.0.0", currentIP)) == 0) {
					//����ں��������򷵻�δ�ܲ�ѯ��������IP��ַ
					memcpy(sendbuf, buffer, bufferLength); 
					unsigned short flag = htons(0x8180);
					memcpy(&sendbuf[2], &flag, sizeof(unsigned short)); //�����ײ���־��
					flag = htons(0x0000);	//���ش��������Ϊ1
					memcpy(&sendbuf[6], &flag, sizeof(unsigned short));
				}
				else {
					memcpy(sendbuf, buffer, bufferLength); 
					unsigned short flag = htons(0x8180);
					memcpy(&sendbuf[2], &flag, sizeof(unsigned short)); //�����ײ���־��
					flag = htons(0x0001);	//���ش��������Ϊ1
					memcpy(&sendbuf[6], &flag, sizeof(unsigned short));

				}
				char answer[16];
				unsigned short Name = htons(0xc00c);  //��������ָ��
				memcpy(answer, &Name, sizeof(unsigned short));
				currenLength += sizeof(unsigned short);

				unsigned short Type = htons(0x0001);  //����
				memcpy(answer + currenLength, &Type, sizeof(unsigned short));
				currenLength += sizeof(unsigned short);

				unsigned short Class = htons(0x0001);  //��
				memcpy(answer + currenLength, &Class, sizeof(unsigned short));
				currenLength += sizeof(unsigned short);

				unsigned long TTL = htonl(0x7b); //����ʱ��
				memcpy(answer + currenLength, &TTL, sizeof(unsigned long));
				currenLength += sizeof(unsigned long);

				unsigned short IPLength = htons(0x0004);  //IP����
				memcpy(answer + currenLength, &IPLength, sizeof(unsigned short));
				currenLength += sizeof(unsigned short);

				unsigned long IP = (unsigned long)inet_addr(currentIP); //���ַ���ʽIPת��Ϊ16������ʽ��IP
				memcpy(answer + currenLength, &IP, sizeof(unsigned long));
				currenLength += sizeof(unsigned long);
				currenLength += bufferLength;
				memcpy(sendbuf + bufferLength, answer, sizeof(answer));

				bufferLength = sendto(localSocket, sendbuf, currenLength, 0, (SOCKADDR*)& client, sizeof(client)); //�����ķ��ͻؿͻ���
				if (debugLevel >= 1)
				{
					if (IPV4) {
						cout << Number << ":  ";
						outPutCurrentTime();
						cout << "  " << "Client" << "  " << "127.0.0.1" << "     " << "1.0.0.127.in-addr.arpa, TYPE 12 ,CLASS 1" << endl;
						Number++;
						cout << Number << ":* ";
						outPutCurrentTime();
						cout << "  " << "Client" << "  " << "127.0.0.1" << "     " << requestURL << endl;
						IPV4 = false;
						Number++;
					}
					else {
						cout << Number << ":  ";
						outPutCurrentTime();
						cout << "  " << "Client" << "  " << "127.0.0.1" << "     " << requestURL << ",  " << "TYPE  28" << ",  " <<  "CLASS  1" << endl;
						Number++;
			
						
						IPV4 = true;
					}
					
					}
				char outBuffer[2];
				if (debugLevel >= 2 ) {
					cout << "----------------------------------------" << endl;
					cout << "buffer :  ";
					for (int i = 0; i < bufferLength; i++) {
						_itoa((unsigned short)buffer[i], outBuffer, 16);
						cout << outBuffer[0] << outBuffer[1] << " ";
					}
					cout << endl;
					cout << "----------------------------------------" << endl;
				}
				
			}

		}
		else {
			strcpy(currentIP, IPCache[CurrentCacheNumber]);
			char sendbuf[MAX_BUF_SIZE];
			memcpy(sendbuf, buffer, bufferLength); 
			unsigned short flag = htons(0x8180);
			memcpy(&sendbuf[2], &flag, sizeof(unsigned short)); //�����ײ���־λ
			flag = htons(0x0001);	//���ûش����
			memcpy(&sendbuf[6], &flag, sizeof(unsigned short));

			int currentLength = 0;
			char answer[16];
			unsigned short Name = htons(0xc00c); //�����ײ���־λ
			memcpy(answer, &Name, sizeof(unsigned short));
			currentLength += sizeof(unsigned short);

			unsigned short Type = htons(0x0001);  //����
			memcpy(answer + currentLength, &Type, sizeof(unsigned short));
			currentLength += sizeof(unsigned short);

			unsigned short Class = htons(0x0001);  //��
			memcpy(answer + currentLength, &Class, sizeof(unsigned short));
			currentLength += sizeof(unsigned short);

			unsigned long TTL = htonl(0x7b); //����ʱ��
			memcpy(answer + currentLength, &TTL, sizeof(unsigned long));
			currentLength += sizeof(unsigned long);

			unsigned short IPLength = htons(0x0004);  //IP����
			memcpy(answer + currentLength, &IPLength, sizeof(unsigned short));
			currentLength += sizeof(unsigned short);

			unsigned long IP = (unsigned long)inet_addr(currentIP); //��IP���ַ�����ת��Ϊ16����
			memcpy(answer + currentLength, &IP, sizeof(unsigned long));
			currentLength += sizeof(unsigned long);
			currentLength += bufferLength;
			memcpy(sendbuf + bufferLength, answer, sizeof(answer));

			bufferLength = sendto(localSocket, sendbuf, currentLength, 0, (SOCKADDR*)& client, sizeof(client)); //��DNS���ķ��ʹ��ͻ���
			if (debugLevel >= 1)
			{
				if (IPV4) {
					cout << Number << ":  ";
					outPutCurrentTime();
					cout << "  " << "Client" << "  " << "127.0.0.1" << "     " << "1.0.0.127.in-addr.arpa, TYPE 12 ,CLASS 1" << endl;
					Number++;
					cout << Number << ":* ";
					outPutCurrentTime();
					cout << "  " << "Client" << "  " << "127.0.0.1" << "     " << requestURL << endl;
					IPV4 = false;
					Number++;
				}
				else {
					cout << Number << ":  ";
					outPutCurrentTime();
					cout << "  " << "Client" << "  " << "127.0.0.1" << "     " << requestURL << ",  " << "TYPE  28"<< ",  " <<  "CLASS  1" << endl;
					Number++;
					IPV4 = true;
				}
			
				}
			
			char outBuffer[2];
			if (debugLevel >= 2 ) {
				cout << "----------------------------------------" << endl;
				cout << "buffer :  ";
				for (int i = 0; i < bufferLength; i++) {
					_itoa((unsigned short)buffer[i], outBuffer, 16);
					cout << outBuffer[0] << outBuffer[1] << " ";
				}
				cout << endl;
				cout << "----------------------------------------" << endl;
			}
			
		}
	}
	
}
int main(int argc, char* argv[]) {
	listTitle();
	//��ȡ�����в���
	readParameters(argc, argv);
	//��ȡIP-�������ձ�
	readIPURLReflectTable();

	//��ʼ��Win Socket����
	WSAStartup(MAKEWORD(2, 2), &WsaData);
	
	//�������غ��ⲿ��Socket
	localSocket = socket(AF_INET, SOCK_DGRAM, 0);
	outSocket = socket(AF_INET, SOCK_DGRAM, 0);

	//��Socket�ӿڸ�Ϊ������ģʽ
	int nonBlock = 1;
	ioctlsocket(outSocket, FIONBIO, (u_long FAR*) & nonBlock);
	ioctlsocket(localSocket, FIONBIO, (u_long FAR*) & nonBlock);

	localName.sin_family = AF_INET;//Address family AF_INET����TCP / IPЭ����
	localName.sin_addr.s_addr = INADDR_ANY;    //���ñ��ص�ַΪ����IP��ַ
	localName.sin_port = htons(53); //����DNS�ӿ�Ϊ53

	externName.sin_family = AF_INET; //Address family AF_INET����TCP / IPЭ����
	externName.sin_addr.s_addr = inet_addr(outDNSServerIP);   //�����ⲿDNS������IP��ַ
	externName.sin_port = htons(53);  //����DNS�ӿ�Ϊ53

	//�����׽��ֵ�ѡ��,������ֱ��ض˿ڱ�ռ�����
	int reuse = 1;
	setsockopt(localSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)& reuse, sizeof(reuse));

	//�󶨸��׽��ֵ�53�˿�
	if (bind(localSocket, (struct sockaddr*) & localName, sizeof(localName)) < 0)
	{
		if (debugLevel >= 1)
			printf("Bind socket port failed.\n");
		exit(1);
	}
	//��ʼ��ID���ձ�
	initTransferTable();

	for (;;)
	{
		receiveFromLocal();
		receiveFromOut();
	}
}