#include <iostream>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <cmath>
#include <vector>
#include <string>
#include <sstream>
using namespace std;
vector<vector<unsigned char> > sBoxVector(16, vector<unsigned char>(16,0));
//vector<vector<unsigned char> > rConVector(11, vector<unsigned char>(4,0));
 unsigned char sBoxArray[256]={
0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
 };
unsigned char Rcon[44] = {
					0x8d,0x0,0x0,0x0, 
/*Round 04*/		0x01,0x0,0x0,0x0, 
/*Round 08*/		0x02,0x0,0x0,0x0, 
/*Round 12*/		0x04,0x0,0x0,0x0, 
/*Round 16*/		0x08,0x0,0x0,0x0, 
/*Round 20*/		0x10,0x0,0x0,0x0, 
/*Round 24*/		0x20,0x0,0x0,0x0, 
/*Round 28*/		0x40,0x0,0x0,0x0, 
/*Round 32*/		0x80,0x0,0x0,0x0, 
/*Round 36*/		0x1b,0x0,0x0,0x0, 
/*Round 40*/		0x36,0x0,0x0,0x0};

vector<vector<unsigned char> > storeArray(vector<unsigned char> byte, int tick, int maxTick){
	vector<vector<unsigned char> > roundArry(4, vector<unsigned char> ( 4, 0 ));

	for (int i =0; i<4; i++){
		for(int j = 0; j<4;j++){
			if (tick<maxTick){
				roundArry[i][j] = byte[tick];
				tick+=1;
			}
			else
				roundArry[i][j] = 0;
		}
	}
	/*
	for (int i =0; i<4; i++){
		for(int j = 0; j<4;j++){
			cout << "["<<roundArry[i][j]<<"]";
		}
		cout<<endl;
	}

	cout<<"--------------------------------------------"<<endl;*/
	return roundArry;
}
vector<unsigned char> rconMe(char count){
	vector<unsigned char> rconResult(4,0);
	unsigned char temp = Rcon[count];
	rconResult[0] = temp;
	for(int i = 1;i<4;i++){
		rconResult[i] = 0;
	}
	return rconResult;
}
/**
*fill_Box() Method
* Takes the already alocated s-box and configure it to be used as a 2-d table.
*/

void fill_Box(){
	int count = 0;
	for (int i =0; i<16; i++){
		for(int j = 0; j<16;j++){
			sBoxVector[i][j] = sBoxArray[count];
			count++;
		}
	}
}
/**
*void print1D Method
* Takes the a 1D vector and print out the contents
*/


void print1D(vector<unsigned char> &print){
	for (int j =0;j<4;j++){
		printf ("%x",(unsigned int)print[j]);
		//cout<<print[j];
	}
	cout <<endl;
}

/**
*void print1D Method
* Takes the a 1D vector and print out the contents
*/


void print2D(vector<vector<unsigned char> > &print){
	for (int i =0;i<4;i++){
		for (int j =0;j<4;j++){
			printf ("%x",(unsigned int)print[i][j]);
			//cout<<print[i][j];
	}
	cout <<endl;
	}
	cout<<"------------------------------------------------------"<<endl;
}
vector<unsigned char> subBytes(vector<unsigned char> subMe){
	for(int i =0;i<4;i++){
		//cout<<((subMe[i]>>4)&0xf)<<(subMe[i]&0xf)<<endl;
		subMe[i] = sBoxVector[((subMe[i]>>4)&0xf)][(subMe[i]&0xf)];
	}
	return subMe;
}
vector<unsigned char> rotate(vector<unsigned char> & inputWord,int numRotate){
	//cout<<"Before State. Iteration of "<<numRotate<<endl;
	//print1D(inputWord);
	for (int j = 0; j<numRotate;j++){
		unsigned char temp = inputWord[0];
		for (int i = 1; i<4;i++)
			inputWord[i-1] = inputWord[i];
		inputWord[3] = temp;
	}
	//cout<<"After State."<<endl;
	//print1D(inputWord);
	return inputWord;
}

unsigned char charMult(unsigned char a, unsigned char b){
	unsigned char returnValue;
		returnValue = (a<<1);
		if((a>>7)==1)
			returnValue^=0x1b;
		if(b==3)
			returnValue^=a;

	return returnValue;
}
vector<unsigned char> shiftColumns(vector<unsigned char> input){
	vector<unsigned char> mixedColumns (4,0);
	unsigned char aMatrix[4][4] = {{0x02, 0x01, 0x01, 0x03}, 
									{0x03, 0x02, 0x01, 0x01}, 
									{0x01, 0x03, 0x02, 0x01},
									{0x01, 0x01, 0x03, 0x02}};
	//for every Produce of 2, we have to shift left by 1 and xor with 0x1b if the left most bit is before shifted is 1
	//
	//for(int col = 0; col<4;col++){
	//	// Multiply the row of A by the column of B to get the row, column of product.
 //       for (int inner = 0; inner < 4; inner++) {
	//		mixedColumns[col] += input[inner] * aMatrix[inner][col] ;
	//	}
	//}

	/*mixedColumns[0] = (0x2*input[0])^(0x3*input[1])^input[2]^input[3];
	mixedColumns[1] = input[0]^(0x2*input[1])^(0x3*input[2])^input[3];
	mixedColumns[2] = input[0]^input[1]^(0x02*input[2])^(0x03*input[3]);
	mixedColumns[3] = (0x03*input[0])^input[1]^input[2]^(0x02*input[3]);*/
	mixedColumns[0] = charMult(input[0],2)^charMult(input[1],0x3)^input[2]^input[3];
	mixedColumns[1] = input[0]^charMult(input[1],2)^charMult(input[2],0x3)^input[3];
	mixedColumns[2] = input[0]^input[1]^charMult(input[2],2)^charMult(input[3],0x03);
	mixedColumns[3] = charMult(input[0],3)^input[1]^input[2]^charMult(input[3],2);

	return mixedColumns;
}
vector<unsigned char> g(vector<unsigned char> &test,int round){
	//print1D(test);
	test = rotate(test,1);
	//print1D(test);
	test = subBytes(test);
	//print1D(test);
	vector<unsigned char> newRecon = rconMe(round);
	for(int i = 0; i< 4;i++)
		test[i] = test[i] ^ newRecon[i];
	//print1D(test);
	return test;
}

vector<vector<unsigned char> > expandKey(vector<vector<unsigned char> > key,int keyLength,int round){
	vector<vector<unsigned char> > expandedKey(4*(round+1),vector<unsigned char> (4,0));
	vector<unsigned char> word(4,0);
	vector<unsigned char> temp(4,0);
	int nK;
	if(keyLength==192)
		nK = 6;
	else if (keyLength==256)
		nK = 8;
	else
		nK = 4;
	int currentRound = 0;
	//Initialize the extended with the regular key
	for(int i = 0; i<4;i++){
		for(int j = 0; j<4;j++){
			expandedKey[i][j] = key[i][j];
		}
		currentRound++;
	}
	//cout<<currentRound<<endl;
	//Start expanding key
	while(currentRound<4*(round+1)){
		temp[0] = expandedKey[currentRound-1][0];
		temp[1] = expandedKey[currentRound-1][1];
		temp[2] = expandedKey[currentRound-1][2];
		temp[3] = expandedKey[currentRound-1][3];
		//print1D(temp);
		if(currentRound %nK == 0){
			temp = g(temp,currentRound);
			//print1D(temp);
		}
		else if(nK>6&&currentRound%nK ==4)
			temp= subBytes(temp);
		//print1D(temp);
		expandedKey[currentRound][0] = expandedKey[currentRound-nK][0]^temp[0];
		expandedKey[currentRound][1] = expandedKey[currentRound-nK][1]^temp[1];
		expandedKey[currentRound][2] = expandedKey[currentRound-nK][2]^temp[2];
		expandedKey[currentRound][3] = expandedKey[currentRound-nK][3]^temp[3];
		currentRound++;
		//print2D(expandedKey);
	}
	//print2D(expandedKey);

	return expandedKey;
}
vector<vector<unsigned char> > encypt(vector<vector<unsigned char> > roundKey, vector<vector<unsigned char> > dataSet,int rounds){
	vector<vector<unsigned char> > encyptedData(4, vector<unsigned char>(4,0));
	vector<unsigned char> tempWord(4,0);
	//Initial encrypt
	for (int i = 0; i<4;i++){
		for(int j = 0; j<4;j++){
			encyptedData[i][j] = dataSet[i][j] ^ roundKey[i][j]; 
		}
	}	
	//print2D(encyptedData);
	for (int r = 1;r < 11;r++){
		//Subbing bytes
		for(int col = 0;col <4;col++){
			tempWord[0] =  encyptedData[col][0];
			tempWord[1] =  encyptedData[col][1];
			tempWord[2] =  encyptedData[col][2];
			tempWord[3] =  encyptedData[col][3];
			tempWord =subBytes(tempWord);
			encyptedData[col][0] = tempWord[0];
			encyptedData[col][1] = tempWord[1];   
			encyptedData[col][2] = tempWord[2]; 
			encyptedData[col][3] = tempWord[3];  
		}
		//print2D(encyptedData);
		//Rotate Rows
		for (int row =0;row<4;row++){
			tempWord[0] =  encyptedData[0][row];
			tempWord[1] =  encyptedData[1][row];
			tempWord[2] =  encyptedData[2][row];
			tempWord[3] =  encyptedData[3][row];
			tempWord = rotate(tempWord,row);
			encyptedData[0][row] = tempWord[0];
			encyptedData[1][row] = tempWord[1];   
			encyptedData[2][row] = tempWord[2]; 
			encyptedData[3][row] = tempWord[3]; 	
		}
	//	print2D(encyptedData);
		//Mix Columns
		if(r!=10)
		for(int col1 = 0; col1 <4;col1++){
			tempWord[0] =  encyptedData[col1][0];
			tempWord[1] =  encyptedData[col1][1];
			tempWord[2] =  encyptedData[col1][2];
			tempWord[3] =  encyptedData[col1][3];
			tempWord = shiftColumns(tempWord);
			encyptedData[col1][0] = tempWord[0];
			encyptedData[col1][1] = tempWord[1];   
			encyptedData[col1][2] = tempWord[2]; 
			encyptedData[col1][3] = tempWord[3];  
		}
		//print2D(encyptedData);
		//add round key
		//round 0 uses key 0-3, round 1 uses 4-7, round 2 uses 8-11
		int currentRoundKey = 4*r;
			for(int i = 0; i <4;i++){
			encyptedData[i][0] = encyptedData[i][0] ^ roundKey[currentRoundKey+i][0]; 
			encyptedData[i][1] = encyptedData[i][1] ^ roundKey[currentRoundKey+i][1]; 
			encyptedData[i][2] = encyptedData[i][2] ^ roundKey[currentRoundKey+i][2]; 
			encyptedData[i][3] = encyptedData[i][3] ^ roundKey[currentRoundKey+i][3]; 
		}
		//print2D(encyptedData);
	}
	
return encyptedData;
}
int main(){
	int choice = 0,keyLength = 128,numRounds = 10, countBytes =0, counter = 0, numCoversion = 0;
	string randomString = "abcdefghijklmnopqrstuvwxyz1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	string defaultKey = "YxK69Rr7Y4TYifBA",stringInput ="";
	bool defString = false;
	//string testKey = "SOME 128 BIT KEY";
	unsigned char testKey[16] ={0x53,0x4f,0x4d, 0x45, 0x20, 0x31, 0x32, 0x38, 0x20, 0x42, 0x49, 0x54 ,0x20, 0x4b,0x45,0x59}; 
	unsigned char testString[16] = {0x41, 0x54, 0x54, 0x41, 0x43, 0x4b, 0x20, 0x41, 0x54, 0x20, 0x44, 0x41, 0x57, 0x4e, 0x21, 0x00};
	vector<unsigned char> key(16);
	vector<vector<unsigned char> > extendedKey(44);
	fill_Box();
	srand((unsigned) time(NULL));
	string menu = "Please select one of the following options:\n\t1: Generate a new 128 bit key\n\t2: Use a default key.\n\t3. Use a test key.\n\t4.Import a key file.\n**Important Info: Key have to be ASCII format.**\n\n(1,2,3,4)?";
	while(true){
		cout << menu <<endl;
		cin>>choice;
		if(choice ==1||choice == 2||choice==3||choice == 4){
			if(choice ==1){
				for (int i =0;i<16;i++)
					key[i]=randomString[rand() % (62)];
			}
			else if (choice ==2){
				for (int i =0;i<16;i++)
					key[i]=defaultKey[i];
			}
			else if (choice ==3){
				for (int i =0;i<16;i++)
					key[i]=testKey[i];
			}
			else{
				cout<<"What is your file name(include the extension)?"<<endl;
				char* file=0;
				string fileKey;
				cin >> file;
				fstream myfile(file,ios::in|ios::out);
				if (myfile.is_open()){
					while ( myfile.good() ){
						getline (myfile,fileKey);
						 //cout << fileKey << endl;
					}
				myfile.close();
				}
				for (int i =0;i<16;i++){
					key[i]=fileKey[i];
				}
			}
			break;
		}
	}
	string menu2 ="Please select one of the following options:\n\t1: Enter your own string.\n\t2. Read from a file.\n\t3.Use a default string.\n(1,2,3)"; 
	cout<<"Your key is: "<<endl;
	for (int i =0;i<16;i++)
		cout<<key[i];
	cout<<endl;
	extendedKey = expandKey(storeArray(key,0,16),keyLength,numRounds);

	while(true){
		cout << menu2 <<endl;
		cin>>choice;
		if(choice ==1||choice == 2||choice==3){
			if(choice ==1){
				cout << "Please input a string to encode:"<<endl;
				cin >> stringInput;
				cout << "You have inputed '"<< stringInput <<"'. Beginning Encryption."<<endl;
			}
			 else if(choice==2){
				cout<<"What is your file name(include the extension)?"<<endl;
				//string file;
				char* file=0;
				cin >> file;
				ifstream myfile(file);
				if (myfile.is_open()){
					while ( myfile.good() ){
						getline (myfile,stringInput);
					}
				myfile.close();
				}
				cout << "You have inputed '"<< stringInput <<"'. Beginning Encryption."<<endl;
			}
			 else{
				 defString = true;
				 //stringstream convert ( abc );

			 }
			break;
		}
	}
	
	//cout << "You have inputed '"<< stringInput <<"'. Beginning Encryption."<<endl;
	countBytes = stringInput.length();
	if (defString ==true){
		countBytes=16;
	}
	vector<unsigned char> plainBytes(countBytes,0);
	if (defString ==false)
	for(int i =0;i<countBytes;i++)
		plainBytes[i] = stringInput[i];
	else
		for(int i =0;i<countBytes;i++)
		plainBytes[i] = testString[i];
	if (countBytes%16 != 0)
		numCoversion = (countBytes/16)+1;
	else
		numCoversion = (countBytes/16);
	vector<vector<vector<unsigned char> > > vectorList(numCoversion, vector<vector<unsigned char> >(4, vector<unsigned char>(4, 0)));
	for (int i=0;i<numCoversion;i++){
		vectorList[i]=storeArray(plainBytes,16*i,countBytes);
	}
	vector<vector<unsigned char> > encryptedBox = encypt(extendedKey,vectorList[0],numRounds);
	string *outputArray;
	outputArray = new string[32];
	int stringCount =0;
	char buffer [32];
	string buffer2;
	
	cout<<"Encrypted data is stored in file 'Encrypted.txt'"<<endl;
	ofstream myfile ("Encrypted.txt");
	if (myfile.is_open())
	for(int i =0; i<4;i++){
		for (int j =0; j<4; j++){
			sprintf (buffer, "%02x",(unsigned int)encryptedBox[i][j]);
			//buffer2+=encryptedBox[i][j];
			//sprintf (buffer2, "%s",(unsigned char)encryptedBox[i][j]);
			 myfile << buffer;
			outputArray[stringCount] = buffer;
			printf ("%s",buffer);
		}
	}
	 else cout << "Unable to open file";
	 cout<<endl;
	 
	// printf ("%s",buffer2);
	 myfile.close();
	int pressEnter;
	cin >> pressEnter;
	cin>> pressEnter;
	
	return 0;
}
